# Copyright IG. All Rights Reserved.
"""Creates the Forensic Mode assets: two materials, five clue data assets, and clue actors in L_Arena.

Run headlessly with Scripts/CreateForensicAssets.bat. Safe to re-run: existing assets are deleted and rebuilt
so the graphs always match this script.
"""
import unreal

MEL = unreal.MaterialEditingLibrary
ATOOLS = unreal.AssetToolsHelpers.get_asset_tools()
EAL = unreal.EditorAssetLibrary


def fresh_asset(name, folder, asset_class, factory):
    path = f"{folder}/{name}"
    if EAL.does_asset_exist(path):
        EAL.delete_asset(path)
    return ATOOLS.create_asset(name, folder, asset_class, factory)


def node(mat, cls, x, y, **props):
    expr = MEL.create_material_expression(mat, cls, x * 220, y * 130)
    for key, value in props.items():
        expr.set_editor_property(key, value)
    return expr


def link(src, dst, dst_input="", src_output=""):
    MEL.connect_material_expressions(src, src_output, dst, dst_input)


def scalar(mat, name, default, x, y):
    return node(mat, unreal.MaterialExpressionScalarParameter, x, y, parameter_name=name, default_value=default)


def const3(mat, r, g, b, x, y):
    return node(mat, unreal.MaterialExpressionConstant3Vector, x, y, constant=unreal.LinearColor(r, g, b, 1.0))


def vector_param(mat, name, rgb, x, y):
    return node(mat, unreal.MaterialExpressionVectorParameter, x, y, parameter_name=name,
                default_value=unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))


def const(mat, v, x, y):
    return node(mat, unreal.MaterialExpressionConstant, x, y, r=v)


def binary(mat, cls, a, b, x, y, in_a="A", in_b="B"):
    n = node(mat, cls, x, y)
    link(a, n, in_a)
    link(b, n, in_b)
    return n


def unary(mat, cls, a, x, y, **props):
    n = node(mat, cls, x, y, **props)
    link(a, n, "")
    return n


# --------------------------------------------------------------------------------------------------------------
# M_ForensicVision: post-process. Desaturated blue world, and clues (custom stencil 1 = unscanned, 2 = scanned)
# drawn as a filled silhouette plus a soft halo, visible through walls. "Alpha" blends the whole effect.
# --------------------------------------------------------------------------------------------------------------
def build_vision():
    """Forensic Mode post-process (V3).

    - Tactical wireframe: edges from scene depth and world normals (4-tap cross), faded with distance.
    - Cold fill: the scene's luminance tinted dark blue, so the world reads as a scan rather than a colour grade.
    - Stencil classes (1 clue, 2 scanned clue, 3 hostile, 4 interactable) filled and haloed in palette colours.
    - Radial reveal: the look spreads outward from RevealCenter up to RevealRadius (driven by the opening pulse).
    - Scan pulse: a glowing ring at PulseRadius around PulseCenter, faded by PulseStrength.
    Alpha blends the whole effect in and out.
    """
    mat = fresh_asset("M_ForensicVision", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)

    def comp_mask(src, x, y, output="", r=False, g=False, b=False, a=False):
        m = node(mat, unreal.MaterialExpressionComponentMask, x, y, r=r, g=g, b=b, a=a)
        MEL.connect_material_expressions(src, output, m, "")
        return m

    def op(cls, a_, b_, x, y, in_a="A", in_b="B"):
        return binary(mat, cls, a_, b_, x, y, in_a, in_b)

    def sat(src, x, y):
        n = node(mat, unreal.MaterialExpressionSaturate, x, y)
        link(src, n, "")
        return n

    def absn(src, x, y):
        n = node(mat, unreal.MaterialExpressionAbs, x, y)
        link(src, n, "")
        return n

    alpha = scalar(mat, "Alpha", 0.0, 0, 0)
    scene = node(mat, unreal.MaterialExpressionSceneTexture, 0, 2, scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    scene_rgb = comp_mask(scene, 1, 2, "Color", r=True, g=True, b=True)

    screen = node(mat, unreal.MaterialExpressionScreenPosition, 0, 6)
    view = node(mat, unreal.MaterialExpressionViewProperty, 0, 7, property=unreal.MaterialExposedViewProperty.MEVP_VIEW_SIZE)

    row = [10]

    def sample(tex_id, dx, dy, channels):
        y = row[0]
        row[0] += 3
        st = node(mat, unreal.MaterialExpressionSceneTexture, 4, y, scene_texture_id=tex_id)
        if dx or dy:
            off = node(mat, unreal.MaterialExpressionConstant2Vector, 1, y, r=dx, g=dy)
            scaled = node(mat, unreal.MaterialExpressionMultiply, 2, y)
            link(off, scaled, "A")
            MEL.connect_material_expressions(view, "InvProperty", scaled, "B")
            uv = node(mat, unreal.MaterialExpressionAdd, 3, y)
            MEL.connect_material_expressions(screen, "ViewportUV", uv, "A")
            link(scaled, uv, "B")
            link(uv, st, "UVs")
        return comp_mask(st, 5, y, "Color", **channels)

    R = {"r": True}
    RGB = {"r": True, "g": True, "b": True}
    D, N, S = unreal.SceneTextureId.PPI_SCENE_DEPTH, unreal.SceneTextureId.PPI_WORLD_NORMAL, unreal.SceneTextureId.PPI_CUSTOM_STENCIL
    taps = [(1.5, 0), (-1.5, 0), (0, 1.5), (0, -1.5)]

    # --- edges ---------------------------------------------------------------------------------------------------
    dc = sample(D, 0, 0, R)
    depth_sum = None
    for i, (dx, dy) in enumerate(taps):
        diff = absn(op(unreal.MaterialExpressionSubtract, sample(D, dx, dy, R), dc, 7, 10 + i), 8, 10 + i)
        depth_sum = diff if depth_sum is None else op(unreal.MaterialExpressionAdd, depth_sum, diff, 9, 10 + i)
    depth_rel = op(unreal.MaterialExpressionDivide, depth_sum, op(unreal.MaterialExpressionMax, dc, const(mat, 1.0, 9, 15), 10, 15), 11, 14)
    edge_depth = sat(op(unreal.MaterialExpressionMultiply, depth_rel, scalar(mat, "DepthEdgeGain", 5.0, 11, 16), 12, 14), 13, 14)

    nc = sample(N, 0, 0, RGB)
    normal_sum = None
    for i, (dx, dy) in enumerate(taps):
        d = op(unreal.MaterialExpressionDotProduct, nc, sample(N, dx, dy, RGB), 7, 30 + i)
        one_minus = node(mat, unreal.MaterialExpressionOneMinus, 8, 30 + i)
        link(d, one_minus, "")
        normal_sum = one_minus if normal_sum is None else op(unreal.MaterialExpressionAdd, normal_sum, one_minus, 9, 30 + i)
    edge_normal = sat(op(unreal.MaterialExpressionMultiply, normal_sum, scalar(mat, "NormalEdgeGain", 1.6, 9, 35), 10, 34), 11, 34)

    far_ratio = op(unreal.MaterialExpressionDivide, dc, const(mat, 9000.0, 11, 22), 12, 22)
    far_one_minus = node(mat, unreal.MaterialExpressionOneMinus, 13, 22)
    link(far_ratio, far_one_minus, "")
    far = sat(far_one_minus, 14, 22)
    edge = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMax, edge_depth, edge_normal, 14, 18), far, 15, 18)

    # --- cold fill -----------------------------------------------------------------------------------------------
    luma = op(unreal.MaterialExpressionDotProduct, scene_rgb, const3(mat, 0.2126, 0.7152, 0.0722, 2, 3), 3, 2)
    fill_color = vector_param(mat, "FillColor", (0.05, 0.11, 0.22), 3, 4)
    base = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, luma, const(mat, 2.2, 4, 3), 5, 3), fill_color, 6, 3)
    edge_color = vector_param(mat, "EdgeColor", (0.35, 0.65, 1.0), 14, 24)
    edge_rgb = op(unreal.MaterialExpressionMultiply, edge, edge_color, 16, 18)

    # --- stencil classes -----------------------------------------------------------------------------------------
    s_c = sample(S, 0, 0, R)
    s_n = None
    for i, (dx, dy) in enumerate([(2, 0), (-2, 0), (0, 2), (0, -2)]):
        v = sample(S, dx, dy, R)
        s_n = v if s_n is None else op(unreal.MaterialExpressionMax, s_n, v, 7, 50 + i)
    class_colors = [vector_param(mat, "UnscannedColor", (1.0, 0.55, 0.1), 8, 56), vector_param(mat, "ScannedColor", (0.35, 0.7, 0.9), 8, 57),
                    vector_param(mat, "HostileColor", (0.9, 0.1, 0.1), 8, 58), vector_param(mat, "InteractColor", (0.88, 0.91, 0.95), 8, 59)]

    def class_color(sv, y):
        total = None
        for k, col in enumerate(class_colors, start=1):
            dist = op(unreal.MaterialExpressionMultiply, absn(op(unreal.MaterialExpressionSubtract, sv, const(mat, float(k), 8, y + k), 9, y + k), 9.5, y + k), const(mat, 2.0, 9, y + k + 0.5), 10, y + k + 0.3)
            om = node(mat, unreal.MaterialExpressionOneMinus, 11, y + k)
            link(dist, om, "")
            wk = sat(om, 12, y + k)
            term = op(unreal.MaterialExpressionMultiply, col, wk, 13, y + k)
            total = term if total is None else op(unreal.MaterialExpressionAdd, total, term, 14, y + k)
        return total

    inside = sat(s_c, 8, 52)
    around = sat(s_n, 8, 53)
    halo_mask = sat(op(unreal.MaterialExpressionSubtract, around, inside, 9, 53), 10, 53)
    fill_rgb = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, class_color(s_c, 60), inside, 16, 60), const(mat, 0.55, 16, 61), 17, 60)
    halo_rgb = op(unreal.MaterialExpressionMultiply, op(unreal.MaterialExpressionMultiply, class_color(s_n, 70), halo_mask, 16, 70), const(mat, 2.2, 16, 71), 17, 70)

    # --- reveal and pulse ----------------------------------------------------------------------------------------
    wp = node(mat, unreal.MaterialExpressionWorldPosition, 0, 80)
    reveal_center = comp_mask(vector_param(mat, "RevealCenter", (0.0, 0.0, 0.0), 0, 81), 1, 81, "", r=True, g=True, b=True)
    reveal_dist = op(unreal.MaterialExpressionDistance, wp, reveal_center, 2, 80)
    reveal = sat(op(unreal.MaterialExpressionDivide, op(unreal.MaterialExpressionSubtract, scalar(mat, "RevealRadius", 1.0e7, 2, 82), reveal_dist, 3, 81), const(mat, 250.0, 3, 82), 4, 81), 5, 81)

    pulse_center = comp_mask(vector_param(mat, "PulseCenter", (0.0, 0.0, 0.0), 0, 84), 1, 84, "", r=True, g=True, b=True)
    pulse_dist = op(unreal.MaterialExpressionDistance, wp, pulse_center, 2, 84)
    ring_off = absn(op(unreal.MaterialExpressionSubtract, pulse_dist, scalar(mat, "PulseRadius", 0.0, 2, 86), 3, 85), 4, 85)
    ring_om = node(mat, unreal.MaterialExpressionOneMinus, 5, 85)
    link(op(unreal.MaterialExpressionDivide, ring_off, const(mat, 220.0, 4, 86), 5, 86), ring_om, "")
    ring = op(unreal.MaterialExpressionMultiply, sat(ring_om, 6, 85), scalar(mat, "PulseStrength", 0.0, 6, 86), 7, 85)
    ring_rgb = op(unreal.MaterialExpressionMultiply, ring, op(unreal.MaterialExpressionMultiply, edge_color, const(mat, 2.5, 7, 87), 8, 87), 9, 85)

    # --- combine -------------------------------------------------------------------------------------------------
    forensic = op(unreal.MaterialExpressionAdd,
                   op(unreal.MaterialExpressionAdd, op(unreal.MaterialExpressionAdd, base, edge_rgb, 18, 3), fill_rgb, 19, 3),
                   halo_rgb, 20, 3)
    mix = op(unreal.MaterialExpressionMultiply, alpha, reveal, 20, 6)
    blended = node(mat, unreal.MaterialExpressionLinearInterpolate, 21, 3)
    link(scene_rgb, blended, "A")
    link(forensic, blended, "B")
    link(mix, blended, "Alpha")
    out = op(unreal.MaterialExpressionAdd, blended, op(unreal.MaterialExpressionMultiply, ring_rgb, alpha, 21, 7), 22, 3)

    MEL.connect_material_property(out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    unreal.log("Built M_ForensicVision")


def build_overlay():
    mat = fresh_asset("M_ForensicOverlay_UI", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    progress = scalar(mat, "Progress", 0.0, 0, 0)
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, 0, 2)
    uv_y = node(mat, unreal.MaterialExpressionComponentMask, 1, 2, r=False, g=True, b=False, a=False)
    link(uv, uv_y, "")
    time = node(mat, unreal.MaterialExpressionTime, 0, 4)

    # Scanlines: thin bright rows scrolling downward.
    rows = binary(mat, unreal.MaterialExpressionMultiply, uv_y, const(mat, 220.0, 1, 3), 2, 2)
    # MotionScale (0 or 1) freezes the scroll for reduced-motion mode.
    motion = scalar(mat, "MotionScale", 1.0, 0, 5)
    scaled_time = binary(mat, unreal.MaterialExpressionMultiply, time, motion, 1, 4)
    scroll = binary(mat, unreal.MaterialExpressionMultiply, scaled_time, const(mat, -7.0, 1, 5), 2, 4)
    phase = binary(mat, unreal.MaterialExpressionAdd, rows, scroll, 3, 3)
    wave = unary(mat, unreal.MaterialExpressionSine, phase, 4, 3)
    unit = binary(mat, unreal.MaterialExpressionAdd, binary(mat, unreal.MaterialExpressionMultiply, wave, const(mat, 0.5, 4, 4), 5, 3),
                  const(mat, 0.5, 5, 4), 6, 3)
    lines = binary(mat, unreal.MaterialExpressionMultiply,
                   binary(mat, unreal.MaterialExpressionPower, unit, const(mat, 10.0, 6, 5), 7, 3, in_a="Base", in_b="Exponent"),
                   const(mat, 0.22, 7, 5), 8, 3)

    # Vignette: darker toward the corners.
    dist = binary(mat, unreal.MaterialExpressionDistance, uv, node(mat, unreal.MaterialExpressionConstant2Vector, 1, 7, r=0.5, g=0.5), 2, 7)
    vignette = unary(mat, unreal.MaterialExpressionSaturate,
                     binary(mat, unreal.MaterialExpressionMultiply,
                            binary(mat, unreal.MaterialExpressionSubtract, dist, const(mat, 0.28, 3, 8), 4, 7),
                            const(mat, 1.7, 4, 8), 5, 7), 6, 7)

    # Wipe: a bright band sweeping up the screen as Progress goes 0 -> 1, fading out at the end.
    target_y = unary(mat, unreal.MaterialExpressionOneMinus, progress, 1, 10)
    delta = unary(mat, unreal.MaterialExpressionAbs, binary(mat, unreal.MaterialExpressionSubtract, uv_y, target_y, 2, 10), 3, 10)
    band = unary(mat, unreal.MaterialExpressionSaturate,
                 unary(mat, unreal.MaterialExpressionOneMinus,
                       binary(mat, unreal.MaterialExpressionMultiply, delta, const(mat, 24.0, 3, 11), 4, 10), 5, 10), 6, 10)
    fade = unary(mat, unreal.MaterialExpressionSaturate,
                 binary(mat, unreal.MaterialExpressionMultiply, unary(mat, unreal.MaterialExpressionOneMinus, progress, 4, 12),
                        const(mat, 3.0, 4, 13), 5, 12), 6, 12)
    band_energy = binary(mat, unreal.MaterialExpressionMultiply, band, fade, 7, 10)

    body = binary(mat, unreal.MaterialExpressionAdd,
                  binary(mat, unreal.MaterialExpressionMultiply, vignette, const(mat, 0.55, 8, 7), 9, 7), lines, 10, 5)
    opacity = unary(mat, unreal.MaterialExpressionSaturate,
                    binary(mat, unreal.MaterialExpressionAdd,
                           binary(mat, unreal.MaterialExpressionMultiply, body, progress, 11, 5),
                           binary(mat, unreal.MaterialExpressionMultiply, band_energy, const(mat, 0.7, 8, 11), 9, 10), 12, 5), 13, 5)

    cyan = vector_param(mat, "Tint", (0.25, 0.85, 1.0), 12, 8)
    emissive = binary(mat, unreal.MaterialExpressionMultiply, cyan,
                      binary(mat, unreal.MaterialExpressionAdd, body, band_energy, 12, 9), 14, 8)

    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    unreal.log("Built M_ForensicOverlay_UI")


# --------------------------------------------------------------------------------------------------------------
# Clue data assets and their placement.
# --------------------------------------------------------------------------------------------------------------
CLUES = [
    ("Ledger", "Torn ledger page", "A page torn from a shipping ledger. Three crates never reached the docks.",
     "/Engine/EngineMaterials/DefaultDiffuse.DefaultDiffuse", (750, 420, 60)),
    ("Footprint", "Muddy footprint", "A size 12 boot print, heading away from the warehouse.",
     "/Engine/EngineMaterials/DefaultWhiteGrid.DefaultWhiteGrid", (-600, 800, 60)),
    ("Casing", "Spent shell casing", "9mm, fired recently. The rounding on the rim points to a custom load.",
     "/Engine/EngineMaterials/Grid.Grid", (350, -1000, 60)),
    ("Keycard", "Dropped keycard", "Maintenance access. Someone went back for it and did not find it.",
     "/Engine/EngineResources/DefaultTexture.DefaultTexture", (-1100, -300, 60)),
    ("Note", "Crumpled note", "Half a phone number and the word 'midnight' underlined twice.",
     "/Engine/EngineMaterials/WeightMapPlaceholderTexture.WeightMapPlaceholderTexture", (0, 1400, 60)),
]


def build_clues():
    assets = []
    for clue_id, title, body, thumb, _ in CLUES:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property("data_asset_class", unreal.ClueDataAsset)
        asset = fresh_asset(f"DA_Clue_{clue_id}", "/Game/Data/Clues", unreal.ClueDataAsset, factory)
        asset.set_editor_property("clue_id", clue_id)
        # Text comes from the code-registered string table, so it is localizable (see ClueStrings.cpp).
        asset.set_editor_property("title", unreal.TextLibrary.text_from_string_table("MvsClues", f"{clue_id}.Title"))
        asset.set_editor_property("description", unreal.TextLibrary.text_from_string_table("MvsClues", f"{clue_id}.Body"))
        if EAL.does_asset_exist(thumb.split(".")[0]):
            asset.set_editor_property("thumbnail", EAL.load_asset(thumb.split(".")[0]))
        else:
            unreal.log_warning(f"Thumbnail missing: {thumb}")
        EAL.save_loaded_asset(asset)
        assets.append(asset)
    return assets


def place_clues(assets):
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    les.load_level("/Game/Maps/L_Arena")
    for actor in eas.get_all_level_actors():
        if isinstance(actor, unreal.ClueActor):
            eas.destroy_actor(actor)
    for asset, spec in zip(assets, CLUES):
        actor = eas.spawn_actor_from_class(unreal.ClueActor, unreal.Vector(*spec[4]))
        actor.set_editor_property("clue", asset)
        actor.set_actor_label(f"Clue_{spec[0]}")
    les.save_current_level()


def build_entry_widget():
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", unreal.ClueEntryWidget)
    bp = fresh_asset("WBP_ClueEntry", "/Game/UI", unreal.WidgetBlueprint, factory)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp)
    unreal.log("Built WBP_ClueEntry")


build_vision()
build_overlay()
build_entry_widget()
place_clues(build_clues())
unreal.log("Forensic assets done")
