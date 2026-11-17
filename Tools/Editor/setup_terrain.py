# Script éditeur (Python) : crée le contenu minimal branché sur les classes C++ FootArcade.
#   - Input : IA_Tir, IMC_Foot
#   - Physique : PM_Ballon
#   - Blueprints : BP_FootBall, BP_FootPlayer, BP_FootPlayerController, BP_FootGameMode
#   - Map : L_Terrain (terrain arcade 40 x 25 m, murs, deux buts, ballon, PlayerStart)
# Idempotent pour les assets ; la map est recréée de zéro à chaque lancement.
# Lancement (éditeur fermé) :
#   UnrealEditor-Cmd.exe FootArcade.uproject -run=pythonscript -script=Tools/Editor/setup_terrain.py -unattended -NullRHI
import unreal, traceback

EAL = unreal.EditorAssetLibrary
AT = unreal.AssetToolsHelpers.get_asset_tools()

ROOT = "/Game/FootArcade"
INPUT_DIR = ROOT + "/Input"
BP_DIR = ROOT + "/Blueprints"
PHYS_DIR = ROOT + "/Physique"
MAP = ROOT + "/Maps/L_Terrain"

# Dimensions du terrain (cm)
LONGUEUR = 4000.0      # axe X, but à but
LARGEUR = 2500.0       # axe Y
MUR_EP = 50.0          # épaisseur des murs
MUR_H = 100.0          # hauteur visible des murs
MUR_INVISIBLE_H = 600.0
BUT_L = 600.0          # largeur du but
BUT_H = 250.0          # hauteur du but
BUT_P = 150.0          # profondeur du but
POTEAU = 12.0          # diamètre des poteaux

errors = []
state = {}

def log(msg):
    unreal.log_warning("[FootSetup] " + str(msg))  # warning = visible dans le log du commandlet

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

# ------------------------------------------------------------------ Physique
def phys_material():
    f = unreal.PhysicalMaterialFactoryNew()
    f.set_editor_property("physical_material_class", unreal.PhysicalMaterial)
    pm = get_or_create("PM_Ballon", PHYS_DIR, unreal.PhysicalMaterial, f)
    pm.set_editor_property("friction", 0.5)
    pm.set_editor_property("restitution", 0.6)
    EAL.save_loaded_asset(pm)
    state["PM_Ballon"] = pm

# ------------------------------------------------------------------ Input
def input_actions():
    ia = get_or_create("IA_Tir", INPUT_DIR, unreal.InputAction, unreal.InputAction_Factory())
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
    imc = get_or_create("IMC_Foot", INPUT_DIR, unreal.InputMappingContext, unreal.InputMappingContext_Factory())
    mv, lk, tir = state["IA_Move"], state["IA_Look"], state["IA_Tir"]
    SW = (unreal.InputModifierSwizzleAxis, {"order": unreal.InputAxisSwizzle.YXZ})
    NEG = (unreal.InputModifierNegate, {})
    NEG_Y = (unreal.InputModifierNegate, {"x": False, "y": True, "z": False})
    DZ = (unreal.InputModifierDeadZone, {})
    maps = [
        # Déplacement : touches physiques WASD (= ZQSD sur AZERTY), flèches, stick gauche
        mapping(imc, mv, "W", [SW]),
        mapping(imc, mv, "S", [SW, NEG]),
        mapping(imc, mv, "A", [NEG]),
        mapping(imc, mv, "D", []),
        mapping(imc, mv, "Up", [SW]),
        mapping(imc, mv, "Down", [SW, NEG]),
        mapping(imc, mv, "Left", [NEG]),
        mapping(imc, mv, "Right", []),
        mapping(imc, mv, "Gamepad_Left2D", [DZ]),
        # Caméra : souris (Y inversé comme le modèle Epic) + stick droit
        mapping(imc, lk, "Mouse2D", [NEG_Y]),
        mapping(imc, lk, "Gamepad_Right2D", [DZ]),
        # Tir : clic gauche, Espace, bouton B / Rond
        mapping(imc, tir, "LeftMouseButton", []),
        mapping(imc, tir, "SpaceBar", []),
        mapping(imc, tir, "Gamepad_FaceButton_Right", []),
    ]
    dkm = imc.get_editor_property("default_key_mappings")
    dkm.set_editor_property("mappings", maps)
    imc.set_editor_property("default_key_mappings", dkm)
    EAL.save_loaded_asset(imc)
    state["IMC_Foot"] = imc

# ------------------------------------------------------------------ Blueprints
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

def bp_ball():
    bp = make_bp("BP_FootBall", unreal.FootBall)
    finish_bp(bp)
    cdo = cdo_of(bp)
    # Ballon arcade : 50 cm de diamètre, 1 kg (lisible à l'écran, plus stable).
    cdo.set_editor_property("mass_kg", 1.0)
    cdo.set_editor_property("linear_damping", 0.3)
    cdo.set_editor_property("angular_damping", 0.5)
    comp = cdo.get_editor_property("ball_mesh")
    comp.set_editor_property("relative_scale3d", unreal.Vector(0.5, 0.5, 0.5))
    body = comp.get_editor_property("body_instance")
    body.set_editor_property("phys_material_override", state["PM_Ballon"])
    comp.set_editor_property("body_instance", body)
    finish_bp(bp)
    state["BP_FootBall"] = bp

def bp_player():
    bp = make_bp("BP_FootPlayer", unreal.FootPlayer)
    finish_bp(bp)
    mesh = cdo_of(bp).get_editor_property("mesh")
    mesh.set_editor_property("skeletal_mesh_asset", load("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple"))
    mesh.set_editor_property("animation_mode", unreal.AnimationMode.ANIMATION_BLUEPRINT)
    mesh.set_editor_property("anim_class", load("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed").generated_class())
    mesh.set_editor_property("relative_location", unreal.Vector(0, 0, -96))
    mesh.set_editor_property("relative_rotation", unreal.Rotator(0, 0, -90))  # Python : (roll, pitch, yaw)
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

# ------------------------------------------------------------------ Map
def level():
    les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
    eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    if EAL.does_asset_exist(MAP):
        # La map existe : on la vide pour la reconstruire.
        if not les.load_level(MAP):
            raise RuntimeError("chargement de la map échoué")
        for a in eas.get_all_level_actors():
            if not isinstance(a, (unreal.WorldSettings, unreal.Brush)):
                eas.destroy_actor(a)
    elif not les.new_level(MAP):
        raise RuntimeError("new_level a échoué")

    cube = load("/Engine/BasicShapes/Cube")
    cyl = load("/Engine/BasicShapes/Cylinder")
    mat_sol = load("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_Gray")
    mat_mur = load("/Game/LevelPrototyping/Materials/MI_PrototypeGrid_TopDark")

    def mesh_actor(label, mesh, loc, scale, rot=None, mat=None, folder="Terrain", hidden=False):
        a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), rot or unreal.Rotator(0, 0, 0))
        if a is None:
            raise RuntimeError("spawn impossible : " + label)
        a.static_mesh_component.set_static_mesh(mesh)
        a.set_actor_label(label)
        a.set_actor_scale3d(unreal.Vector(*scale))
        a.set_folder_path(folder)
        if mat:
            a.static_mesh_component.set_material(0, mat)
        if hidden:
            a.set_actor_hidden_in_game(True)
        return a

    def box(label, cx, cy, cz, sx, sy, sz, **kw):
        # Les dimensions sont en cm ; le cube de l'engine fait 100 cm.
        return mesh_actor(label, cube, (cx, cy, cz), (sx / 100.0, sy / 100.0, sz / 100.0), **kw)

    L2, W2 = LONGUEUR / 2, LARGEUR / 2

    # Sol (dessus à z = 0), un peu plus grand que l'aire de jeu
    box("Sol", 0, 0, -50, LONGUEUR + 600, LARGEUR + 600, 100, mat=mat_sol)

    # Murs latéraux (visibles bas + invisibles hauts)
    for s, nom in ((1, "Gauche"), (-1, "Droit")):
        y = s * (W2 + MUR_EP / 2)
        box("Mur_" + nom, 0, y, MUR_H / 2, LONGUEUR + 2 * MUR_EP, MUR_EP, MUR_H, mat=mat_mur)
        box("MurInvisible_" + nom, 0, y, MUR_INVISIBLE_H / 2, LONGUEUR + 2 * MUR_EP, MUR_EP, MUR_INVISIBLE_H,
            folder="Terrain/Invisible", hidden=True)

    # Murs de fond, coupés par l'ouverture du but
    seg = W2 - BUT_L / 2
    for sx, camp in ((1, "Est"), (-1, "Ouest")):
        x = sx * (L2 + MUR_EP / 2)
        for sy in (1, -1):
            y = sy * (BUT_L / 2 + seg / 2)
            cote = "G" if sy > 0 else "D"
            box("Mur_%s_%s" % (camp, cote), x, y, MUR_H / 2, MUR_EP, seg, MUR_H, mat=mat_mur)
            box("MurInvisible_%s_%s" % (camp, cote), x, y, MUR_INVISIBLE_H / 2, MUR_EP, seg, MUR_INVISIBLE_H,
                folder="Terrain/Invisible", hidden=True)
        # Au-dessus de la barre transversale
        hz = MUR_INVISIBLE_H - BUT_H
        box("MurInvisible_%s_DessusBut" % camp, x, 0, BUT_H + hz / 2, MUR_EP, BUT_L, hz,
            folder="Terrain/Invisible", hidden=True)

        # But : deux poteaux + barre, et une cage fermée derrière la ligne
        f = "Buts/But_" + camp
        r = POTEAU / 100.0
        for sy in (1, -1):
            mesh_actor("Poteau_%s_%s" % (camp, "G" if sy > 0 else "D"), cyl,
                       (sx * L2, sy * BUT_L / 2, BUT_H / 2), (r, r, BUT_H / 100.0), folder=f)
        mesh_actor("Barre_" + camp, cyl, (sx * L2, 0, BUT_H), (r, r, (BUT_L + POTEAU) / 100.0),
                   rot=unreal.Rotator(90, 0, 0), folder=f)
        xb = sx * (L2 + BUT_P / 2)
        box("Cage_Fond_" + camp, sx * (L2 + BUT_P + 5), 0, BUT_H / 2, 10, BUT_L, BUT_H, mat=mat_mur, folder=f)
        box("Cage_Toit_" + camp, xb, 0, BUT_H + 5, BUT_P, BUT_L, 10, mat=mat_mur, folder=f)
        for sy in (1, -1):
            box("Cage_Cote_%s_%s" % (camp, "G" if sy > 0 else "D"), xb, sy * (BUT_L / 2 + 5), BUT_H / 2,
                BUT_P, 10, BUT_H, mat=mat_mur, folder=f)

    # Éclairage et ciel (lumières mobiles : pas de build de lumière nécessaire)
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(0, -50, 30))
    sun.set_actor_label("Soleil"); sun.set_folder_path("Ambiance")
    lc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    lc.set_mobility(unreal.ComponentMobility.MOVABLE)
    lc.set_editor_property("atmosphere_sun_light", True)
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1200))
    sky.set_actor_label("LumiereCiel"); sky.set_folder_path("Ambiance")
    sc = sky.get_component_by_class(unreal.SkyLightComponent)
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_editor_property("real_time_capture", True)
    for cls, label in ((unreal.SkyAtmosphere, "Atmosphere"), (unreal.ExponentialHeightFog, "Brouillard")):
        a = eas.spawn_actor_from_class(cls, unreal.Vector(0, 0, 0))
        a.set_actor_label(label); a.set_folder_path("Ambiance")

    # Départ du joueur et ballon au centre
    ps = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(-600, 0, 100), unreal.Rotator(0, 0, 0))
    ps.set_actor_label("Depart_Joueur")
    ball = eas.spawn_actor_from_class(state["BP_FootBall"].generated_class(), unreal.Vector(0, 0, 30))
    ball.set_actor_label("Ballon")

    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property("default_game_mode", state["BP_FootGameMode"].generated_class())
    if not les.save_current_level():
        raise RuntimeError("sauvegarde de la map échouée")

for fn in (phys_material, input_actions, input_context, bp_ball, bp_player, bp_controller, bp_gamemode, level):
    step(fn)

if errors:
    unreal.log_error("[FootSetup] TERMINE AVEC ERREURS : " + ", ".join(errors))
else:
    log("TERMINE SANS ERREUR")
