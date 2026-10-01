# Copyright IG. All Rights Reserved.
"""Builds /Game/Maps/L_Arena: a wet night rooftop with a lit skyline (V1 visual pass).

Everything is generated: geometry from engine / LevelPrototyping meshes, materials from expressions, no textures.
Lumen stays off (project decision), so the look relies on direct lights, SSR on wet surfaces, volumetric fog and
grading. Re-running rebuilds the map from scratch; run CreateForensicAssets afterwards to place the clues again
(Scripts/BuildLevel.bat does both).
"""
import math
import random

import unreal

MAP = "/Game/Maps/L_Arena"
MAT_DIR = "/Game/Materials/Environment"

MEL = unreal.MaterialEditingLibrary
EAL = unreal.EditorAssetLibrary
ATOOLS = unreal.AssetToolsHelpers.get_asset_tools()
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

CUBE = EAL.load_asset("/Engine/BasicShapes/Cube")
CYLINDER = EAL.load_asset("/Engine/BasicShapes/Cylinder")
CHAMFER = EAL.load_asset("/Game/LevelPrototyping/Meshes/SM_ChamferCube") if EAL.does_asset_exist("/Game/LevelPrototyping/Meshes/SM_ChamferCube") else CUBE

INTERACTABLE = 4  # EMvsStencil::Interactable

# Clue positions (see CreateForensicAssets.py); props keep clear of them so clues stay visible.
CLUE_SPOTS = [(750, 420), (-600, 800), (350, -1000), (-1100, -300), (0, 1400)]


# ----------------------------------------------------------------------------------------------------------------
# Material helpers
# ----------------------------------------------------------------------------------------------------------------
def fresh_material(name, domain=None):
    """Creates the material, or clears an existing one in place. Materials used by the (auto-loaded) startup level or
    the hero's defaults cannot be deleted and recreated, so they are rebuilt rather than replaced."""
    path = f"{MAT_DIR}/{name}"
    if EAL.does_asset_exist(path):
        mat = EAL.load_asset(path)
        MEL.delete_all_material_expressions(mat)
    else:
        mat = ATOOLS.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())
    if domain is not None:
        mat.set_editor_property("material_domain", domain)
    else:
        # LevelPrototyping meshes are Nanite; without the flag they fall back to the default material.
        mat.set_editor_property("used_with_nanite", True)
    return mat


def node(mat, cls, x, y, **props):
    expr = MEL.create_material_expression(mat, cls, x * 220, y * 130)
    for key, value in props.items():
        expr.set_editor_property(key, value)
    return expr


def link(src, dst, dst_input="", src_output=""):
    MEL.connect_material_expressions(src, src_output, dst, dst_input)


def const(mat, v, x, y):
    return node(mat, unreal.MaterialExpressionConstant, x, y, r=v)


def color(mat, rgb, x, y):
    return node(mat, unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def vparam(mat, name, rgb, x, y):
    return node(mat, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def sparam(mat, name, v, x, y):
    return node(mat, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=v)


def op(mat, cls, a, b, x, y, in_a="A", in_b="B"):
    n = node(mat, cls, x, y)
    link(a, n, in_a)
    link(b, n, in_b)
    return n


def un(mat, cls, src, x, y, **props):
    n = node(mat, cls, x, y, **props)
    link(src, n, "")
    return n


def lerp(mat, a, b, alpha, x, y):
    n = node(mat, unreal.MaterialExpressionLinearInterpolate, x, y)
    link(a, n, "A")
    link(b, n, "B")
    link(alpha, n, "Alpha")
    return n


def mask(mat, src, x, y, r=False, g=False, b=False, a=False):
    return un(mat, unreal.MaterialExpressionComponentMask, src, x, y, r=r, g=g, b=b, a=a)


def hash2(mat, v2, x, y):
    """frac(sin(dot(v, (12.9898, 78.233))) * 43758.5453): a cheap per-cell random in [0,1)."""
    k = node(mat, unreal.MaterialExpressionConstant2Vector, x, y + 1, r=12.9898, g=78.233)
    d = op(mat, unreal.MaterialExpressionDotProduct, v2, k, x + 1, y)
    s = un(mat, unreal.MaterialExpressionSine, d, x + 2, y, period=6.2831853)
    m = op(mat, unreal.MaterialExpressionMultiply, s, const(mat, 43758.5453, x + 2, y + 1), x + 3, y)
    return un(mat, unreal.MaterialExpressionFrac, m, x + 4, y)


def world_xy(mat, scale, x, y):
    wp = node(mat, unreal.MaterialExpressionWorldPosition, x, y)
    xy = mask(mat, wp, x + 1, y, r=True, g=True)
    return op(mat, unreal.MaterialExpressionMultiply, xy, const(mat, scale, x + 1, y + 1), x + 2, y)


def finish(mat):
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    return mat


# ----------------------------------------------------------------------------------------------------------------
# Materials
# ----------------------------------------------------------------------------------------------------------------
def build_wet_concrete():
    """Dark concrete with large tonal blotches and rain puddles (low roughness, darker albedo)."""
    mat = fresh_material("M_WetConcrete")
    wp = node(mat, unreal.MaterialExpressionWorldPosition, 0, 0)
    blotch = node(mat, unreal.MaterialExpressionNoise, 1, 0, scale=0.0025, levels=2, output_min=0.0, output_max=1.0, quality=1)
    link(wp, blotch, "Position")
    puddles = node(mat, unreal.MaterialExpressionNoise, 1, 2, scale=0.0012, levels=1, output_min=-1.0, output_max=1.0, quality=1)
    link(wp, puddles, "Position")
    # Sharpen the puddle field into a mask; the Z-up normal keeps puddles off walls.
    wet = un(mat, unreal.MaterialExpressionSaturate,
             op(mat, unreal.MaterialExpressionMultiply,
                op(mat, unreal.MaterialExpressionSubtract, puddles, sparam(mat, "PuddleCoverage", -0.05, 1, 3), 2, 2),
                const(mat, 6.0, 2, 3), 3, 2), 4, 2)
    normal = node(mat, unreal.MaterialExpressionVertexNormalWS, 2, 4)
    up = un(mat, unreal.MaterialExpressionSaturate,
            op(mat, unreal.MaterialExpressionMultiply,
               op(mat, unreal.MaterialExpressionSubtract, mask(mat, normal, 3, 4, b=True), const(mat, 0.7, 3, 5), 4, 4),
               const(mat, 4.0, 4, 5), 5, 4), 6, 4)
    wet_up = op(mat, unreal.MaterialExpressionMultiply, wet, up, 7, 2)

    dry = lerp(mat, vparam(mat, "DryDark", (0.035, 0.037, 0.042), 2, 0), vparam(mat, "DryLight", (0.09, 0.09, 0.095), 2, 1), blotch, 3, 0)
    base = lerp(mat, dry, op(mat, unreal.MaterialExpressionMultiply, dry, const(mat, 0.45, 4, 1), 5, 1), wet_up, 8, 0)
    rough = lerp(mat, const(mat, 0.82, 7, 3), const(mat, 0.06, 7, 4), wet_up, 8, 3)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(rough, "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(const(mat, 0.5, 8, 5), "", unreal.MaterialProperty.MP_SPECULAR)
    return finish(mat)


def build_metal():
    mat = fresh_material("M_DarkMetal")
    MEL.connect_material_property(vparam(mat, "Tint", (0.05, 0.055, 0.06), 0, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(const(mat, 1.0, 0, 1), "", unreal.MaterialProperty.MP_METALLIC)
    MEL.connect_material_property(sparam(mat, "Roughness", 0.3, 0, 2), "", unreal.MaterialProperty.MP_ROUGHNESS)
    return finish(mat)


def build_emissive():
    """Neon tubes, lamp heads and signs: unlit colour times an intensity."""
    mat = fresh_material("M_Emissive")
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    e = op(mat, unreal.MaterialExpressionMultiply, vparam(mat, "Color", (1.0, 0.2, 0.6), 0, 0), sparam(mat, "Intensity", 12.0, 0, 1), 1, 0)
    MEL.connect_material_property(e, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return finish(mat)


def build_skyline():
    """Distant building facades: near-black with a grid of randomly lit, randomly tinted windows."""
    mat = fresh_material("M_Skyline")
    wp = node(mat, unreal.MaterialExpressionWorldPosition, 0, 0)
    # Windows are laid out on the facade plane: horizontal = X+Y (works for both facade orientations), vertical = Z.
    wx = op(mat, unreal.MaterialExpressionAdd, mask(mat, wp, 1, 0, r=True), mask(mat, wp, 1, 1, g=True), 2, 0)
    wz = mask(mat, wp, 1, 2, b=True)
    grid = op(mat, unreal.MaterialExpressionMultiply,
              op(mat, unreal.MaterialExpressionAppendVector, wx, wz, 3, 0),
              node(mat, unreal.MaterialExpressionConstant2Vector, 3, 1, r=1.0 / 110.0, g=1.0 / 150.0), 4, 0)
    cell = un(mat, unreal.MaterialExpressionFloor, grid, 5, 0)
    local = un(mat, unreal.MaterialExpressionFrac, grid, 5, 1)
    rnd = hash2(mat, cell, 6, 0)
    # Window rectangle inside each cell.
    lx = mask(mat, local, 6, 3, r=True)
    ly = mask(mat, local, 6, 4, g=True)
    def band(v, lo, hi, x, y):
        a = un(mat, unreal.MaterialExpressionSaturate, op(mat, unreal.MaterialExpressionMultiply, op(mat, unreal.MaterialExpressionSubtract, v, const(mat, lo, x, y + 1), x + 1, y), const(mat, 40.0, x + 1, y + 1), x + 2, y), x + 3, y)
        b = un(mat, unreal.MaterialExpressionSaturate, op(mat, unreal.MaterialExpressionMultiply, op(mat, unreal.MaterialExpressionSubtract, const(mat, hi, x, y + 2), v, x + 1, y + 2), const(mat, 40.0, x + 1, y + 3), x + 2, y + 2), x + 3, y + 2)
        return op(mat, unreal.MaterialExpressionMultiply, a, b, x + 4, y)
    window = op(mat, unreal.MaterialExpressionMultiply, band(lx, 0.2, 0.8, 7, 3), band(ly, 0.25, 0.75, 7, 7), 12, 3)
    lit = un(mat, unreal.MaterialExpressionSaturate,
             op(mat, unreal.MaterialExpressionMultiply,
                op(mat, unreal.MaterialExpressionSubtract, rnd, sparam(mat, "LitThreshold", 0.84, 11, 1), 12, 0),
                const(mat, 50.0, 12, 1), 13, 0), 14, 0)
    warm = lerp(mat, color(mat, (1.0, 0.62, 0.3), 13, 5), color(mat, (0.55, 0.75, 1.0), 13, 6),
                un(mat, unreal.MaterialExpressionFrac, op(mat, unreal.MaterialExpressionMultiply, rnd, const(mat, 7.0, 13, 8), 14, 7), 15, 7), 16, 5)
    emissive = op(mat, unreal.MaterialExpressionMultiply,
                  op(mat, unreal.MaterialExpressionMultiply, window, lit, 15, 3),
                  op(mat, unreal.MaterialExpressionMultiply, warm, sparam(mat, "WindowIntensity", 0.25, 16, 7), 17, 5), 18, 3)
    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(color(mat, (0.012, 0.013, 0.016), 17, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(const(mat, 0.6, 17, 1), "", unreal.MaterialProperty.MP_ROUGHNESS)
    return finish(mat)


def build_suit():
    """Hero suit: near-black armour with a faint cold sheen, so the silhouette reads against the wet roof."""
    mat = fresh_material("M_Suit")
    mat.set_editor_property("used_with_skeletal_mesh", True)
    fres = node(mat, unreal.MaterialExpressionFresnel, 0, 2, exponent=4.0)
    base = lerp(mat, vparam(mat, "Base", (0.008, 0.009, 0.012), 0, 0), vparam(mat, "Rim", (0.03, 0.035, 0.05), 0, 1), fres, 1, 0)
    MEL.connect_material_property(base, "", unreal.MaterialProperty.MP_BASE_COLOR)
    MEL.connect_material_property(sparam(mat, "Roughness", 0.55, 1, 2), "", unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.connect_material_property(const(mat, 0.3, 1, 3), "", unreal.MaterialProperty.MP_SPECULAR)
    return finish(mat)


def build_rain():
    """Post-process rain: thin, slanted, scrolling streaks from a hashed cell grid. MotionScale 0 freezes it."""
    mat = fresh_material("M_Rain_PP", unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)
    scene = node(mat, unreal.MaterialExpressionSceneTexture, 0, 0, scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    scene_rgb = mask(mat, scene, 1, 0, r=True, g=True, b=True)
    MEL.connect_material_expressions(scene, "Color", scene_rgb, "")

    screen = node(mat, unreal.MaterialExpressionScreenPosition, 0, 3)
    uv = node(mat, unreal.MaterialExpressionComponentMask, 1, 3, r=True, g=True)
    MEL.connect_material_expressions(screen, "ViewportUV", uv, "")
    time = op(mat, unreal.MaterialExpressionMultiply, node(mat, unreal.MaterialExpressionTime, 0, 6), sparam(mat, "MotionScale", 1.0, 0, 7), 1, 6)

    total = None
    # Two layers at different scales and speeds give depth.
    for i, (cols, rows, speed, slant, weight) in enumerate([(160.0, 9.0, 2.6, 0.12, 1.0), (90.0, 5.0, 1.7, 0.08, 0.6)]):
        y0 = 10 + i * 12
        slanted = op(mat, unreal.MaterialExpressionAdd, uv,
                     op(mat, unreal.MaterialExpressionMultiply,
                        op(mat, unreal.MaterialExpressionAppendVector, mask(mat, uv, 2, y0 + 1, g=True), const(mat, 0.0, 2, y0 + 2), 3, y0 + 1),
                        const(mat, slant, 3, y0 + 2), 4, y0 + 1), 5, y0)
        scroll = op(mat, unreal.MaterialExpressionAdd, slanted,
                    op(mat, unreal.MaterialExpressionAppendVector, const(mat, 0.0, 4, y0 + 4),
                       op(mat, unreal.MaterialExpressionMultiply, time, const(mat, -speed, 4, y0 + 5), 5, y0 + 4), 6, y0 + 4), 7, y0)
        grid = op(mat, unreal.MaterialExpressionMultiply, scroll, node(mat, unreal.MaterialExpressionConstant2Vector, 7, y0 + 1, r=cols, g=rows), 8, y0)
        cell = un(mat, unreal.MaterialExpressionFloor, grid, 9, y0)
        local = un(mat, unreal.MaterialExpressionFrac, grid, 9, y0 + 1)
        rnd = hash2(mat, cell, 10, y0)
        present = un(mat, unreal.MaterialExpressionSaturate,
                     op(mat, unreal.MaterialExpressionMultiply, op(mat, unreal.MaterialExpressionSubtract, rnd, const(mat, 0.9, 14, y0 + 2), 15, y0), const(mat, 30.0, 15, y0 + 2), 16, y0), 17, y0)
        # Thin in X, a fading tail in Y.
        lx = mask(mat, local, 10, y0 + 3, r=True)
        thin = un(mat, unreal.MaterialExpressionSaturate,
                  op(mat, unreal.MaterialExpressionSubtract, const(mat, 1.0, 11, y0 + 4),
                     op(mat, unreal.MaterialExpressionMultiply, un(mat, unreal.MaterialExpressionAbs, op(mat, unreal.MaterialExpressionSubtract, lx, const(mat, 0.5, 11, y0 + 5), 12, y0 + 3), 13, y0 + 3), const(mat, 9.0, 13, y0 + 4), 14, y0 + 3), 15, y0 + 3), 16, y0 + 3)
        tail = mask(mat, local, 10, y0 + 6, g=True)
        streak = op(mat, unreal.MaterialExpressionMultiply, op(mat, unreal.MaterialExpressionMultiply, thin, tail, 17, y0 + 3), present, 18, y0)
        layer = op(mat, unreal.MaterialExpressionMultiply, streak, const(mat, weight, 18, y0 + 1), 19, y0)
        total = layer if total is None else op(mat, unreal.MaterialExpressionAdd, total, layer, 20, y0)

    amount = op(mat, unreal.MaterialExpressionMultiply, total, sparam(mat, "RainAmount", 0.18, 19, 3), 21, 3)
    tint = color(mat, (0.7, 0.8, 0.95), 21, 5)
    out = op(mat, unreal.MaterialExpressionAdd, scene_rgb, op(mat, unreal.MaterialExpressionMultiply, tint, amount, 22, 4), 23, 0)
    MEL.connect_material_property(out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return finish(mat)


def instance(parent, name, vectors=None, scalars=None):
    path = f"{MAT_DIR}/{name}"
    if EAL.does_asset_exist(path):
        mi = EAL.load_asset(path)
    else:
        mi = ATOOLS.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    MEL.set_material_instance_parent(mi, parent)
    for k, v in (vectors or {}).items():
        MEL.set_material_instance_vector_parameter_value(mi, k, unreal.LinearColor(v[0], v[1], v[2], 1.0))
    for k, v in (scalars or {}).items():
        MEL.set_material_instance_scalar_parameter_value(mi, k, v)
    MEL.update_material_instance(mi)
    EAL.save_loaded_asset(mi)
    return mi


# ----------------------------------------------------------------------------------------------------------------
# Level
# ----------------------------------------------------------------------------------------------------------------
def rgb_color(rgb):
    return unreal.Color(r=int(rgb[0] * 255), g=int(rgb[1] * 255), b=int(rgb[2] * 255), a=255)


def rotator(pitch=0.0, yaw=0.0, roll=0.0):
    # unreal.Rotator's positional order is (roll, pitch, yaw); always go through keywords.
    return unreal.Rotator(pitch=pitch, yaw=yaw, roll=roll)


def mesh_actor(mesh, loc, scale, mat, label, rot=None, shadows=True, stencil=0):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc))
    smc = a.static_mesh_component
    smc.set_static_mesh(mesh)
    smc.set_material(0, mat)
    smc.set_editor_property("cast_shadow", shadows)
    if stencil:
        # Forensic Mode colours custom-depth objects by stencil class (see Gameplay/ForensicTypes.h).
        smc.set_editor_property("render_custom_depth", True)
        smc.set_editor_property("custom_depth_stencil_value", stencil)
    a.set_actor_scale3d(unreal.Vector(*scale))
    if rot:
        a.set_actor_rotation(rotator(*rot), False)
    a.set_actor_label(label)
    a.set_folder_path("Environment")
    return a


def box(loc, size, mat, label, rot=None, mesh=None, shadows=True, stencil=0):
    """Box by centre and full size in cm (engine cube is 100 cm)."""
    return mesh_actor(mesh or CUBE, loc, (size[0] / 100.0, size[1] / 100.0, size[2] / 100.0), mat, label, rot, shadows, stencil)


def clear_of_clues(x, y, radius):
    return all(math.hypot(x - cx, y - cy) > radius for cx, cy in CLUE_SPOTS)


def point_light(loc, rgb, intensity, radius, label, volumetric=3.0, shadows=False):
    a = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(*loc))
    c = a.get_component_by_class(unreal.PointLightComponent)
    configure_light(c, rgb, intensity, radius, volumetric, shadows)
    a.set_actor_label(label)
    a.set_folder_path("Lighting")
    return a


def configure_light(c, rgb, intensity, radius, volumetric, shadows):
    """Setter functions (not raw property writes) so radius and units take effect on the render proxy."""
    c.set_intensity_units(unreal.LightUnits.CANDELAS)
    c.set_intensity(intensity)
    c.set_light_color(unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    c.set_attenuation_radius(radius)
    c.set_editor_property("attenuation_radius", radius)  # the setter alone does not persist in the saved level
    c.set_cast_shadows(shadows)
    c.set_volumetric_scattering_intensity(volumetric)


def spot_light(loc, rot, rgb, intensity, radius, cone, label):
    a = eas.spawn_actor_from_class(unreal.SpotLight, unreal.Vector(*loc))
    a.set_actor_rotation(rotator(*rot), False)
    c = a.get_component_by_class(unreal.SpotLightComponent)
    configure_light(c, rgb, intensity, radius, 1.5, True)
    c.set_outer_cone_angle(cone)
    c.set_inner_cone_angle(cone * 0.55)
    a.set_actor_label(label)
    a.set_folder_path("Lighting")
    return a


def build_level(mats):
    concrete, metal, skyline = mats["concrete"], mats["metal"], mats["skyline"]
    sodium_glow, neon_pink, neon_cyan, cold_glow = mats["sodium"], mats["pink"], mats["cyan"], mats["cold"]
    rng = random.Random(7)

    # L_Arena is the editor startup map, so it is already loaded and cannot be deleted and recreated. Open it and
    # clear every actor instead, so each build starts from nothing and old actors never linger.
    unreal.EditorAssetLibrary.make_directory("/Game/Maps")
    if EAL.does_asset_exist(MAP):
        les.load_level(MAP)
        for actor in eas.get_all_level_actors():
            if not isinstance(actor, unreal.WorldSettings):
                eas.destroy_actor(actor)
    else:
        les.new_level(MAP)
    remaining = [a.get_actor_label() for a in eas.get_all_level_actors() if not isinstance(a, unreal.WorldSettings)]
    if remaining:
        raise RuntimeError(f"Level not empty before rebuild: {remaining[:10]}")

    # Rooftop slab, parapet, and a raised deck on one side for height variation.
    box((0, 0, -50), (4400, 4400, 100), concrete, "Roof")
    for i, (x, y, sx, sy) in enumerate([(2185, 0, 30, 4400), (-2185, 0, 30, 4400), (0, 2185, 4400, 30), (0, -2185, 4400, 30)]):
        box((x, y, 55), (sx, sy, 110), concrete, f"Parapet_{i}")
        box((x, y, 113), (sx + 10, sy + 10, 6), metal, f"ParapetCap_{i}")
    box((-1650, 1650, 60), (1000, 1000, 120), concrete, "RaisedDeck")
    box((-1650, 1050, 30), (400, 200, 60), concrete, "DeckStep")

    # Stairwell hut with a lit door.
    box((1500, 1500, 180), (500, 400, 360), concrete, "Stairwell", stencil=INTERACTABLE)
    box((1500, 1500, 364), (520, 420, 8), metal, "StairwellRoof")
    box((1248, 1500, 110), (6, 140, 220), cold_glow, "StairwellDoorGlow", shadows=False)
    point_light((1180, 1500, 250), (0.75, 0.85, 1.0), 120, 900, "DoorLight", shadows=True)

    # Water tank on legs.
    for dx, dy in [(-140, -140), (140, -140), (-140, 140), (140, 140)]:
        box((-1500 + dx, -1500 + dy, 150), (20, 20, 300), metal, "TankLeg")
    mesh_actor(CYLINDER, (-1500, -1500, 450), (4.0, 4.0, 3.0), metal, "WaterTank", stencil=INTERACTABLE)

    # AC units and vents, scattered clear of clues and the spawn.
    placed = 0
    while placed < 14:
        x, y = rng.uniform(-1900, 1900), rng.uniform(-1900, 1900)
        if math.hypot(x, y) < 450 or not clear_of_clues(x, y, 320) or (x > 1100 and y > 1100) or (x < -1100 and y < -1100) or (x < -1100 and y > 1100):
            continue
        w, d, h = rng.uniform(140, 320), rng.uniform(140, 260), rng.uniform(90, 200)
        box((x, y, h / 2), (w, d, h), metal if placed % 3 else concrete, f"Vent_{placed}", rot=(0, rng.choice([0, 90, 15, -20]), 0), mesh=CHAMFER)
        placed += 1

    # Antenna mast with a red beacon.
    box((1900, -1850, 500), (16, 16, 1000), metal, "Antenna")
    box((1900, -1850, 1005), (24, 24, 24), mats["red"], "AntennaBeacon", shadows=False)
    point_light((1900, -1850, 1005), (1.0, 0.1, 0.05), 60, 700, "BeaconLight")

    # Sodium lamp poles at the parapet corners: the main warm pools on the roof.
    for i, (x, y, yaw) in enumerate([(2000, 2000, 225), (-2000, 2000, 315), (-2000, -2000, 45), (2000, -2000, 135), (0, -2050, 90), (2050, 0, 180), (-700, -600, 0), (650, 950, 180)]):
        box((x, y, 250), (18, 18, 500), metal, f"LampPole_{i}")
        arm = (x + 120 * math.cos(math.radians(yaw)), y + 120 * math.sin(math.radians(yaw)), 495)
        box(arm, (60, 30, 16), sodium_glow, f"LampHead_{i}", rot=(0, yaw, 0), shadows=False)
        spot_light((arm[0], arm[1], arm[2] - 12), (-90, 0, 0), (1.0, 0.55, 0.22), 9000, 1800, 58, f"SodiumLamp_{i}")

    # Skyline: a ring of dark towers with lit windows, below and above roof level, fading into fog.
    for i in range(46):
        ang = rng.uniform(0, 2 * math.pi)
        dist = rng.uniform(3600, 9500)
        cx, cy = math.cos(ang) * dist, math.sin(ang) * dist
        w, d = rng.uniform(700, 1800), rng.uniform(700, 1800)
        top = rng.uniform(-400, 3200) if dist > 5000 else rng.uniform(-600, 1200)
        bottom = -4000
        box((cx, cy, (top + bottom) / 2), (w, d, top - bottom), skyline, f"Tower_{i}", rot=(0, math.degrees(ang) % 90, 0), shadows=False)

    # Neon signs on a few near towers, facing the roof, each with a coloured spill light.
    for i, (ang, dist, z, mat, _rgb) in enumerate([(0.35, 3700, 700, neon_pink, (1.0, 0.15, 0.55)), (2.2, 3900, 350, neon_cyan, (0.2, 0.85, 1.0)),
                                                 (3.9, 3800, 900, neon_pink, (1.0, 0.15, 0.55)), (5.2, 4100, 500, neon_cyan, (0.2, 0.85, 1.0))]):
        cx, cy = math.cos(ang) * dist, math.sin(ang) * dist
        box((cx, cy, z), (900, 700, 1800), skyline, f"SignTower_{i}", shadows=False)
        face = (cx - math.cos(ang) * 360, cy - math.sin(ang) * 360, z + 500)
        box(face, (20, 600, 90), mat, f"NeonSign_{i}", rot=(0, math.degrees(ang), 0), shadows=False)
        # No spill light: at this distance the sign reads through its emissive and bloom alone.

    # Moonlight, sky light, fog.
    moon = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000))
    moon.set_actor_rotation(rotator(pitch=-32, yaw=35), False)
    mc = moon.get_component_by_class(unreal.DirectionalLightComponent)
    mc.set_editor_property("intensity", 2.0)
    mc.set_editor_property("light_color", rgb_color((0.6, 0.72, 1.0)))  # cold blue-white
    mc.set_editor_property("volumetric_scattering_intensity", 0.15)
    moon.set_actor_label("Moon")
    moon.set_folder_path("Lighting")

    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 2000))
    sc = sky.get_component_by_class(unreal.SkyLightComponent)
    # The captured scene is full of neon and lit windows, which would tint all ambient light; keep the sky light a
    # faint cold fill and out of the volumetric fog.
    sc.set_editor_property("intensity", 0.15)
    sc.set_editor_property("light_color", rgb_color((0.35, 0.55, 1.0)))  # cold blue
    sc.set_editor_property("volumetric_scattering_intensity", 0.0)
    sc.set_editor_property("lower_hemisphere_color", unreal.LinearColor(0.004, 0.005, 0.008, 1.0))
    sky.set_actor_label("SkyAmbient")
    sky.set_folder_path("Lighting")

    fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -200))
    fc = fog.get_component_by_class(unreal.ExponentialHeightFogComponent)
    fc.set_editor_property("fog_density", 0.03)
    fc.set_editor_property("fog_height_falloff", 0.12)
    fc.set_editor_property("fog_inscattering_luminance", unreal.LinearColor(0.02, 0.03, 0.06, 1.0))
    fc.set_editor_property("fog_max_opacity", 0.97)
    fc.set_editor_property("start_distance", 300.0)
    fc.set_editor_property("enable_volumetric_fog", True)
    fc.set_editor_property("volumetric_fog_extinction_scale", 0.35)
    fc.set_editor_property("volumetric_fog_distance", 5000.0)
    fog.set_actor_label("NightFog")
    fog.set_folder_path("Lighting")

    # Grading: fixed exposure (no auto-exposure breathing at night), cool shadows, grain, vignette, rain pass.
    ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0))
    ppv.set_editor_property("unbound", True)
    ppv.set_actor_label("NightGrade")
    ppv.set_folder_path("Lighting")
    s = ppv.settings
    def setp(name, value):
        s.set_editor_property("override_" + name, True)
        s.set_editor_property(name, value)
    setp("auto_exposure_method", unreal.AutoExposureMethod.AEM_MANUAL)
    setp("auto_exposure_apply_physical_camera_exposure", False)
    setp("auto_exposure_bias", 0.5)
    setp("bloom_intensity", 0.9)
    setp("bloom_threshold", 0.8)
    setp("vignette_intensity", 0.55)
    setp("film_grain_intensity", 0.22)
    setp("scene_fringe_intensity", 0.35)
    setp("color_saturation", unreal.Vector4(0.7, 0.72, 0.8, 1.0))
    setp("color_gain_shadows", unreal.Vector4(0.85, 0.95, 1.15, 1.0))
    setp("color_contrast", unreal.Vector4(1.1, 1.1, 1.1, 1.0))
    setp("screen_space_reflection_intensity", 100.0)
    setp("screen_space_reflection_quality", 60.0)
    setp("screen_space_reflection_max_roughness", 0.5)
    setp("ambient_occlusion_intensity", 0.6)
    blendables = unreal.WeightedBlendables()
    blendables.set_editor_property("array", [unreal.WeightedBlendable(weight=1.0, object=mats["rain"])])
    s.set_editor_property("weighted_blendables", blendables)
    ppv.set_editor_property("settings", s)

    start = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, -250, 110))
    start.set_actor_rotation(rotator(yaw=90), False)

    les.save_current_level()


unreal.EditorAssetLibrary.make_directory(MAT_DIR)
emissive = build_emissive()
materials = {
    "concrete": build_wet_concrete(),
    "metal": build_metal(),
    "skyline": build_skyline(),
    "rain": build_rain(),
    "suit": build_suit(),
    "sodium": instance(emissive, "MI_Glow_Sodium", {"Color": (1.0, 0.55, 0.2)}, {"Intensity": 4.0}),
    "pink": instance(emissive, "MI_Neon_Pink", {"Color": (1.0, 0.1, 0.5)}, {"Intensity": 5.0}),
    "cyan": instance(emissive, "MI_Neon_Cyan", {"Color": (0.1, 0.8, 1.0)}, {"Intensity": 5.0}),
    "cold": instance(emissive, "MI_Glow_Cold", {"Color": (0.7, 0.85, 1.0)}, {"Intensity": 0.6}),
    "red": instance(emissive, "MI_Glow_Red", {"Color": (1.0, 0.05, 0.02)}, {"Intensity": 30.0}),
}
build_level(materials)
unreal.log(f"Created night L_Arena ({len(eas.get_all_level_actors())} actors)")
