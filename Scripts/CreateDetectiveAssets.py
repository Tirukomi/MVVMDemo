"""Creates the Detective Mode assets: two materials, five clue data assets, and clue actors in L_Arena.

Run headlessly with Scripts/CreateDetectiveAssets.bat. Safe to re-run: existing assets are deleted and rebuilt
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
# M_DetectiveVision: post-process. Desaturated blue world, and clues (custom stencil 1 = unscanned, 2 = scanned)
# drawn as a filled silhouette plus a soft halo, visible through walls. "Alpha" blends the whole effect.
# --------------------------------------------------------------------------------------------------------------
def build_vision():
    mat = fresh_asset("M_DetectiveVision", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_SCENE_COLOR_AFTER_TONEMAPPING)

    scene = node(mat, unreal.MaterialExpressionSceneTexture, 0, 0,
                 scene_texture_id=unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)
    alpha = scalar(mat, "Alpha", 0.0, 0, 8)

    # Stylised world: grey scaled by a blue tint, slightly dimmed.
    # SceneTexture "Color" is float4; keep just RGB so it can mix with the float3 effect colours.
    scene_rgb = node(mat, unreal.MaterialExpressionComponentMask, 0.5, 1, r=True, g=True, b=True, a=False)
    MEL.connect_material_expressions(scene, "Color", scene_rgb, "")
    desat = node(mat, unreal.MaterialExpressionDesaturation, 1, 0)
    link(scene_rgb, desat, "")
    tint = const3(mat, 0.30, 0.55, 0.95, 1, 1)
    world = binary(mat, unreal.MaterialExpressionMultiply, desat, tint, 2, 0)

    # Stencil sampling: centre plus four neighbours two pixels away (for the halo).
    screen = node(mat, unreal.MaterialExpressionScreenPosition, 0, 4)
    inv_size = node(mat, unreal.MaterialExpressionViewProperty, 0, 5,
                    property=unreal.MaterialExposedViewProperty.MEVP_VIEW_SIZE)

    def stencil_at(offset_x, offset_y, row):
        st = node(mat, unreal.MaterialExpressionSceneTexture, 4, row,
                  scene_texture_id=unreal.SceneTextureId.PPI_CUSTOM_STENCIL)
        if offset_x or offset_y:
            off = node(mat, unreal.MaterialExpressionConstant2Vector, 2, row, r=offset_x, g=offset_y)
            scaled = node(mat, unreal.MaterialExpressionMultiply, 3, row)
            link(off, scaled, "A")
            MEL.connect_material_expressions(inv_size, "InvProperty", scaled, "B")
            uv = node(mat, unreal.MaterialExpressionAdd, 3, row + 0.5)
            MEL.connect_material_expressions(screen, "ViewportUV", uv, "A")
            link(scaled, uv, "B")
            link(uv, st, "UVs")
        red = node(mat, unreal.MaterialExpressionComponentMask, 5, row, r=True, g=False, b=False, a=False)
        MEL.connect_material_expressions(st, "Color", red, "")
        return red

    centre = stencil_at(0, 0, 10)
    neighbours = [stencil_at(2, 0, 12), stencil_at(-2, 0, 14), stencil_at(0, 2, 16), stencil_at(0, -2, 18)]

    nmax = neighbours[0]
    for i, n in enumerate(neighbours[1:]):
        nmax = binary(mat, unreal.MaterialExpressionMax, nmax, n, 6 + i, 12)

    inside = unary(mat, unreal.MaterialExpressionSaturate, centre, 6, 10)            # 1 on a clue
    around = unary(mat, unreal.MaterialExpressionSaturate, nmax, 9, 12)              # 1 next to a clue
    halo_mask = unary(mat, unreal.MaterialExpressionSaturate,
                      binary(mat, unreal.MaterialExpressionSubtract, around, inside, 10, 12), 11, 12)

    # Unscanned = orange, scanned = green. (stencil - 1) is 0 or 1.
    orange = const3(mat, 1.0, 0.55, 0.1, 7, 9)
    green = const3(mat, 0.2, 1.0, 0.4, 7, 10)
    one = const(mat, 1.0, 7, 11)

    def pick(stencil_value, row):
        scanned = unary(mat, unreal.MaterialExpressionSaturate,
                        binary(mat, unreal.MaterialExpressionSubtract, stencil_value, one, 8, row), 9, row)
        lerp = node(mat, unreal.MaterialExpressionLinearInterpolate, 10, row)
        link(orange, lerp, "A")
        link(green, lerp, "B")
        link(scanned, lerp, "Alpha")
        return lerp

    fill_color = pick(centre, 20)
    halo_color = pick(nmax, 22)

    fill = binary(mat, unreal.MaterialExpressionMultiply, fill_color, inside, 12, 20)
    glow_strength = const(mat, 2.2, 11, 23)
    halo_scaled = binary(mat, unreal.MaterialExpressionMultiply, halo_color, halo_mask, 12, 22)
    halo = binary(mat, unreal.MaterialExpressionMultiply, halo_scaled, glow_strength, 13, 22)

    detective = binary(mat, unreal.MaterialExpressionAdd,
                       binary(mat, unreal.MaterialExpressionAdd, world, fill, 14, 0), halo, 15, 0)

    out = node(mat, unreal.MaterialExpressionLinearInterpolate, 16, 0)
    link(scene_rgb, out, "A")
    link(detective, out, "B")
    link(alpha, out, "Alpha")

    MEL.connect_material_property(out, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    unreal.log("Built M_DetectiveVision")


# --------------------------------------------------------------------------------------------------------------
# M_DetectiveOverlay_UI: full-screen UI material. Scanlines, vignette and a sweeping wipe band, all scaled by
# "Progress" (the view model's transition alpha).
# --------------------------------------------------------------------------------------------------------------
def build_overlay():
    mat = fresh_asset("M_DetectiveOverlay_UI", "/Game/Materials", unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_UI)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)

    progress = scalar(mat, "Progress", 0.0, 0, 0)
    uv = node(mat, unreal.MaterialExpressionTextureCoordinate, 0, 2)
    uv_y = node(mat, unreal.MaterialExpressionComponentMask, 1, 2, r=False, g=True, b=False, a=False)
    link(uv, uv_y, "")
    time = node(mat, unreal.MaterialExpressionTime, 0, 4)

    # Scanlines: thin bright rows scrolling downward.
    rows = binary(mat, unreal.MaterialExpressionMultiply, uv_y, const(mat, 220.0, 1, 3), 2, 2)
    scroll = binary(mat, unreal.MaterialExpressionMultiply, time, const(mat, -7.0, 1, 4), 2, 4)
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

    cyan = const3(mat, 0.25, 0.85, 1.0, 12, 8)
    emissive = binary(mat, unreal.MaterialExpressionMultiply, cyan,
                      binary(mat, unreal.MaterialExpressionAdd, body, band_energy, 12, 9), 14, 8)

    MEL.connect_material_property(emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    MEL.connect_material_property(opacity, "", unreal.MaterialProperty.MP_OPACITY)
    MEL.recompile_material(mat)
    EAL.save_loaded_asset(mat)
    unreal.log("Built M_DetectiveOverlay_UI")


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
        asset.set_editor_property("title", title)
        asset.set_editor_property("description", body)
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
unreal.log("Detective assets done")
