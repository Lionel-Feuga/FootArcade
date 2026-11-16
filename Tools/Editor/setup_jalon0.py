# Script éditeur (Python) : crée les assets qui branchent les classes C++ sur le contenu.
# Idempotent : peut être relancé, il met à jour les assets existants.
# Lancement hors éditeur :
#   UnrealEditor-Cmd.exe FootArcade.uproject -run=pythonscript -script=Tools/Editor/setup_jalon0.py -unattended -NullRHI
import unreal, traceback

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()
SDS = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
SDL = unreal.SubobjectDataBlueprintFunctionLibrary

INPUT_DIR = "/Game/FootArcade/Input"
BP_DIR = "/Game/FootArcade/Blueprints"
MAP = "/Game/FootArcade/Maps/L_Terrain"

errors = []
def log(msg):
    unreal.log("[FootSetup] " + str(msg))

def step(fn):
    try:
        fn()
        log("OK " + fn.__name__)
    except Exception:
        errors.append(fn.__name__)
        unreal.log_error("[FootSetup] ERREUR %s\n%s" % (fn.__name__, traceback.format_exc()))

def load(path):
    a = EAL.load_asset(path)
    if a is None:
        raise RuntimeError("introuvable : " + path)
    return a

def get_or_create(name, folder, cls, factory):
    path = folder + "/" + name
    if EAL.does_asset_exist(path):
        return load(path)
    return AT.create_asset(name, folder, cls, factory)

def make_key(name):
    k = unreal.Key()
    k.set_editor_property("key_name", name)
    return k

state = {}

# ---------------------------------------------------------------- Inputs
def input_actions():
    f = unreal.InputAction_Factory() if hasattr(unreal, "InputAction_Factory") else None
    ia = get_or_create("IA_Tir", INPUT_DIR, unreal.InputAction, f)
    ia.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    EAL.save_loaded_asset(ia)
    state["IA_Tir"] = ia
    state["IA_Move"] = load("/Game/Input/Actions/IA_Move")
    state["IA_Look"] = load("/Game/Input/Actions/IA_Look")

def mapping(imc, action, key, mods=()):
    m = unreal.EnhancedActionKeyMapping()
    m.set_editor_property("action", action)
    m.set_editor_property("key", make_key(key))
    objs = []
    for cls, props in mods:
        o = unreal.new_object(cls, outer=imc)
        for k, v in props.items():
            o.set_editor_property(k, v)
        objs.append(o)
    m.set_editor_property("modifiers", objs)
    return m

def input_context():
    f = unreal.InputMappingContext_Factory() if hasattr(unreal, "InputMappingContext_Factory") else None
    imc = get_or_create("IMC_Foot", INPUT_DIR, unreal.InputMappingContext, f)
    mv, lk, tir = state["IA_Move"], state["IA_Look"], state["IA_Tir"]
    SW = (unreal.InputModifierSwizzleAxis, {"order": unreal.InputAxisSwizzle.YXZ})
    NEG = (unreal.InputModifierNegate, {})
    NEG_Y = (unreal.InputModifierNegate, {"x": False, "y": True, "z": False})
    DZ = (unreal.InputModifierDeadZone, {})
    maps = [
        # Déplacement clavier (ZQSD sur AZERTY = touches physiques WASD) + flèches + stick gauche
        mapping(imc, mv, "W", [SW]),
        mapping(imc, mv, "S", [SW, NEG]),
        mapping(imc, mv, "A", [NEG]),
        mapping(imc, mv, "D", []),
        mapping(imc, mv, "Up", [SW]),
        mapping(imc, mv, "Down", [SW, NEG]),
        mapping(imc, mv, "Left", [NEG]),
        mapping(imc, mv, "Right", []),
        mapping(imc, mv, "Gamepad_Left2D", [DZ]),
        # Caméra : souris (Y inversé comme le template) + stick droit
        mapping(imc, lk, "Mouse2D", [NEG_Y]),
        mapping(imc, lk, "Gamepad_Right2D", [DZ]),
        # Tir : clic gauche, barre espace, bouton B / Rond
        mapping(imc, tir, "LeftMouseButton", []),
        mapping(imc, tir, "SpaceBar", []),
        mapping(imc, tir, "Gamepad_FaceButton_Right", []),
    ]
    dkm = imc.get_editor_property("default_key_mappings")
    dkm.set_editor_property("mappings", maps)
    imc.set_editor_property("default_key_mappings", dkm)
    EAL.save_loaded_asset(imc)
    state["IMC_Foot"] = imc

# ---------------------------------------------------------------- Blueprints
def make_bp(name, parent):
    path = BP_DIR + "/" + name
    if EAL.does_asset_exist(path):
        return load(path)
    f = unreal.BlueprintFactory()
    f.set_editor_property("parent_class", parent)
    return AT.create_asset(name, BP_DIR, None, f)

def finish_bp(bp):
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    EAL.save_loaded_asset(bp)

def cdo_of(bp):
    return unreal.get_default_object(bp.generated_class())

def old_ball_component():
    """Retrouve le StaticMeshComponent du BP_Ballon de Lionel pour reprendre ses réglages."""
    old = load("/Game/FootArcade/Blueprints/BP_Ballon")
    for h in SDS.k2_gather_subobject_data_for_blueprint(old):
        d = SDL.get_data(h)
        o = SDL.get_associated_object(d) if hasattr(SDL, "get_associated_object") else SDL.get_object(d)
        log("BP_Ballon composant : %s" % (o.get_class().get_name() if o else None))
        if isinstance(o, unreal.StaticMeshComponent):
            return o
    return None

def bp_ball():
    bp = make_bp("BP_FootBall", unreal.FootBall)
    finish_bp(bp)
    cdo = cdo_of(bp)
    comp = cdo.get_ball_mesh() if hasattr(cdo, "get_ball_mesh") else cdo.get_editor_property("ball_mesh")
    src = old_ball_component()
    if src is not None:
        # Reprend le mesh, l'échelle et la physique du BP_Ballon existant.
        comp.set_editor_property("static_mesh", src.get_editor_property("static_mesh"))
        comp.set_editor_property("relative_scale3d", src.get_editor_property("relative_scale3d"))
        body = src.get_editor_property("body_instance")
        cdo.set_editor_property("linear_damping", body.get_editor_property("linear_damping"))
        cdo.set_editor_property("angular_damping", body.get_editor_property("angular_damping"))
        if body.get_editor_property("override_mass"):
            cdo.set_editor_property("mass_kg", body.get_editor_property("mass_in_kg_override"))
        log("BP_Ballon repris : scale %s, damping %s/%s" % (src.get_editor_property("relative_scale3d"),
            body.get_editor_property("linear_damping"), body.get_editor_property("angular_damping")))
    body = comp.get_editor_property("body_instance")
    body.set_editor_property("phys_material_override", load("/Game/FootArcade/Physique/PM_Ballon"))
    comp.set_editor_property("body_instance", body)
    finish_bp(bp)
    state["BP_FootBall"] = bp

def bp_player():
    bp = make_bp("BP_FootPlayer", unreal.FootPlayer)
    finish_bp(bp)
    cdo = cdo_of(bp)
    mesh = cdo.get_editor_property("mesh")
    mesh.set_editor_property("skeletal_mesh_asset", load("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"))
    mesh.set_editor_property("anim_class", load("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed").generated_class())
    mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
    mesh.set_editor_property("relative_location", unreal.Vector(0, 0, -96))
    mesh.set_editor_property("relative_rotation", unreal.Rotator(0, 0, -90))  # (roll, pitch, yaw) en Python
    finish_bp(bp)
    state["BP_FootPlayer"] = bp

def bp_controller():
    bp = make_bp("BP_FootPlayerController", unreal.FootPlayerController)
    finish_bp(bp)
    cdo = cdo_of(bp)
    cdo.set_editor_property("default_mapping_context", state["IMC_Foot"])
    cdo.set_editor_property("move_action", state["IA_Move"])
    cdo.set_editor_property("look_action", state["IA_Look"])
    cdo.set_editor_property("kick_action", state["IA_Tir"])
    finish_bp(bp)
    state["BP_FootPlayerController"] = bp

def bp_gamemode():
    bp = make_bp("BP_FootGameMode", unreal.FootGameMode)
    finish_bp(bp)
    cdo = cdo_of(bp)
    cdo.set_editor_property("default_pawn_class", state["BP_FootPlayer"].generated_class())
    cdo.set_editor_property("player_controller_class", state["BP_FootPlayerController"].generated_class())
    finish_bp(bp)
    state["BP_FootGameMode"] = bp

# ---------------------------------------------------------------- Niveau
def level():
    unreal.EditorLoadingAndSavingUtils.load_map(MAP)
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", state["BP_FootGameMode"].generated_class())
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    ball_cls = state["BP_FootBall"].generated_class()
    actors = eas.get_all_level_actors()
    if not any(a.get_class() == ball_cls for a in actors):
        loc = unreal.Vector(700, 140, 30)
        for a in actors:
            if a.get_class().get_name() == "BP_Ballon_C":
                loc = a.get_actor_location()
                eas.destroy_actor(a)  # l'asset BP_Ballon est conservé, seule l'instance est remplacée
        nb = eas.spawn_actor_from_class(ball_cls, loc)
        nb.set_actor_label("Ballon")
    unreal.EditorLoadingAndSavingUtils.save_current_level()

for fn in (input_actions, input_context, bp_ball, bp_player, bp_controller, bp_gamemode, level):
    step(fn)

if errors:
    unreal.log_error("[FootSetup] TERMINE AVEC ERREURS : " + ", ".join(errors))
else:
    log("TERMINE SANS ERREUR")
