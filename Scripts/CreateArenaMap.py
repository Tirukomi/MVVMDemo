"""Creates /Game/Maps/L_Arena (grey-box arena). Run via Scripts/CreateArenaMap.bat."""
import unreal

MAP = "/Game/Maps/L_Arena"
les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

unreal.EditorAssetLibrary.make_directory("/Game/Maps")
if unreal.EditorAssetLibrary.does_asset_exist(MAP):
    unreal.log("L_Arena already exists")
else:
    les.new_level(MAP)
    cube = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")

    def spawn(loc, scale, label):
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, loc)
        a.static_mesh_component.set_static_mesh(cube)
        a.set_actor_scale3d(scale)
        a.set_actor_label(label)
        return a

    spawn(unreal.Vector(0, 0, -50), unreal.Vector(40, 40, 1), "Floor")
    for i, (x, y, sx, sy) in enumerate([(2000, 0, 1, 40), (-2000, 0, 1, 40), (0, 2000, 40, 1), (0, -2000, 40, 1)]):
        spawn(unreal.Vector(x, y, 200), unreal.Vector(sx, sy, 5), f"Wall_{i}")
    for i, (x, y) in enumerate([(600, 400), (-500, 700), (300, -800), (-900, -300)]):
        spawn(unreal.Vector(x, y, 50), unreal.Vector(2, 2, 2), f"Cover_{i}")

    eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 100))
    eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000)).set_actor_rotation(unreal.Rotator(0, -45, 30), False)
    eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1000))
    eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    les.save_current_level()
    unreal.log("Created L_Arena")
