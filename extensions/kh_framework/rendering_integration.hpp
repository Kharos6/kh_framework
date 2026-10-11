#pragma once

#define KH_SAFE_RELEASE(p) if (p) { (p)->Release(); (p) = nullptr; }

// FBX/texture third-party units (ufbx, mikktspace, stb_image) + <fstream>/
// <memory>: included by the framework header ahead of this file.
// KH_WC_COPY (kh_wc_copy) uses __cpuid, _mm_mfence and _mm_stream_load_si128:
// <intrin.h> and <smmintrin.h> come from the framework header ahead of this file.

// The integration headers are CRLF-only, tab-free ASCII. Shaders are plain
// .hlsl RCDATA resources (kh_shaders.rc).
namespace RenderIntegration {

// ===========================================================================
// SQF COMMAND REFERENCE - the script surface of the renderer's three headers, in full. The
// description strings registered in sqf_integration.hpp are one or two
// sentences each; everything a command accepts is written HERE, and an edit
// to a command's parser (sqf_integration.hpp: add_render3d_sqf,
// kh_apply_render3d_prop, update_post_fx_sqf, add_physics_affector_sqf, ...)
// or to a property's meaning is an edit to its entry below. User .hlsl files
// have their own reference: the USER SHADER CONTRACT in cb.hlsl.
// getRenderStats' fields are explained where its result is built
// (get_render_stats_sqf, sqf_integration.hpp).
//
// A number outside the range a key or value gives below is clamped to it,
// silently (the solvers and passes are stable over those ranges) - except
// a blend mode or an effect given as a number, a material's texture
// animation (uvScale, uvScroll, uvWarp, flipbook) and vertexBound, which
// are refused with a report outside theirs.
//
// KH_PLAYER_ONLY: only a machine with an interface has graphics (hasInterface,
// framework.hpp's g_is_player). On a dedicated server or a headless client
// every command below does nothing - no parse, no report, nothing created -
// and returns its empty value: '' for a STRING, false for a BOOL, [] for an
// ARRAY.
//
// Every mission start clears what the mission before made - its meshes,
// passes and physics affectors, and the settings back to their defaults -
// in Intercept's pre-init, which runs as a CBA XEH preInit handler. A
// command run before it at the same start (a preInit function the game
// happens to call ahead of Intercept's) is cleared with the rest, without a
// report: issue the first commands from postInit or later.
// ===========================================================================
//
// ---- FILE PATHS ------------------------------------------------------------
// Every file a command below names - a mesh's .fbx, a material's .hlsl and
// its textures, an effect's .hlsl or .cube and its textures - is looked up in
// Documents\Arma 3\kh_framework\rendering, then in every mod's 'rendering'
// folder: the path as given under each folder, then its file name alone at
// the top of each. A path with ONE leading slash - \x\kh\addons\main\m.fbx,
// or /x/kh/addons/main/m.fbx - is instead the file at that engine path inside
// the loaded PBOs, the game's and every mod's (the PBO's prefix, then the
// path inside it), read straight from the PBO: no rendering folder, no
// file-name fallback, case and / against \ alike. PATH_CONFINE: the path as
// given must name a file BELOW the folder - a drive or rooted path, two
// leading slashes (a network path), a '..' segment, a ':' or a device name
// is refused before any file is touched, and only its file name alone is
// looked for at the top of each folder (search_mod_folders.hpp's note).
//
// A texture - a material's map, an effect's user0 .. user5 - is a png, jpg,
// jpeg, tga, bmp or dds file, or the game's own .paa (or .pac, the same file),
// so a texture in the game's or a mod's PBOs can be named by its engine path
// (\a3\...\x_co.paa). A .paa keeps its own mip levels and, for DXT1 / DXT3 /
// DXT5, its compression - except where the texture converter swizzled its
// channels (every _nohq / _nofhq / _nshq / _novhq / _nopx normal map, _sky,
// _adshq, _dt, _detail): the swizzle is undone, so the texture reads as the
// picture it was made from (a _nohq's X comes back out of alpha), and such a
// texture is held uncompressed - 4x a DXT5's memory, 8x a DXT1's (_dt,
// _detail). So is a DXT file - a .paa's, or a .dds's BC1 / BC2 / BC3 - whose
// full-size level is not a multiple of 4 pixels on a side (a 2 x 2, a 256 x 1
// strip): a block format needs that (such a BC7 .dds is refused, with the
// reason). A
// channel the converter dropped does not come back: an alpha reads opaque, a
// colour channel what the file stores in its place.
// Operation Flashpoint's palette-indexed files are not read.
//
// ---- STRING = addRender3D ARRAY --------------------------------------------
// [[x,y,zASL], rotation, mesh]. rotation = nil | yaw | [pitch, yaw, roll] |
// [vectorDir, vectorUp]; mesh = builtin name | registry index | .fbx path
// (nil = box). [vectorDir, vectorUp] is the pair setVectorDirAndUp takes, in
// world axes: the direction is kept and the up made square to it (a zero or
// parallel pair is refused). The mesh rotations below - updateRender3D's
// rotation and attachRotation, a chain's endRotation - take the same four
// forms.
//
// Either the position or the rotation slot may instead be a game OBJECT, and
// the mesh then follows that object's transform every frame (the two are
// independent and may name different objects). A character (kind of Man) is
// followed through a hidden helper attached to it - as the object of a plain
// binding, a rotation object, or the parent of a skeletal binding - so the
// mesh stays with it in vehicles; the helper is shared by every binding on
// that character and deleted with the last one. While the character rides
// in a vehicle, whatever is followed on it - the helper, a memory point, a
// skeletal binding's bones, a chain's end - is placed and turned as the
// character is drawn, every frame (the game trails a crew member's
// attached objects behind it).
//
// In the Eden editor, which does not keep a drawn copy of every entity,
// everything followed - an object, a memory point, a skeletal binding's
// bones, a chain's end - is read from the simulation instead; on anything
// moving that can differ from where the engine draws it.
//
// Position may also be [object, memoryPoint] to follow a model memory point
// (bone); rotation is then REQUIRED and must be true (follow the bone's
// rotation) or false.
//
// Position [object, true] binds the mesh's skeleton to the object: the
// model's origin sits on the object's, every bone whose name matches one of
// the object's memory points (case-insensitive) follows that point, a bone
// with no match follows its parent bone, and a bone with neither stays with
// the object; the rest pose must put the bones on the memory points. What no
// bone moves - a vertex with no weight, or all of a mesh with no skeleton or
// none of whose bones match (a builtin, say) - stays with the object too.
// Rotation is then true (the default: the model turns with the object) or
// false (it keeps its rotation while the bones keep their positions relative
// to the object). The mesh is drawn at its authored scale, its bounds follow
// the pose, and at range it draws a coarser LOD level as any mesh does, every
// level skinned (lodLock true keeps the full-detail one). Many bindings of one
// mesh draw together, instanced, each in its own pose (getRenderStats:
// skinPooled, skinInstanced), wherever a mesh of their kind would. With
// clothSimulation on it is the cloth's to draw: the bones carry what
// kh_cloth_pin holds (a partly pinned vertex
// is pulled toward where its bones put it) and leave what kh_cloth_sim frees
// to the simulation.
//
// [object, false] attaches to the object as a plain OBJECT does.
//
// Beside a plain object in the position slot (the object itself or [object,
// false]) the rotation slot also takes true - the mesh turns with that
// object, as naming it again would - or false, no rotation (as nil); a bare
// number there is still a yaw.
//
// The mesh is removed automatically if either the object or its memory-point
// proxy is deleted.
//
// Far from the map origin the game keeps a memory point's world position to
// float precision - steps of about 0.5 mm at 4-8 km, 1 mm at 8-16 km and
// 2 mm past 16 km, horizontally - so a point that barely moves jitters by
// that step (the game's own attached objects do the same). A memory-point
// binding, the bones of a skeletal binding and a chain's end on a memory
// point follow a smoothed track of the point instead, always within one such
// step of where the game reports it; a jump of more than 64 steps in one
// frame (a teleport) is taken exactly. An attached mesh is also placed finer
// than a world float, so its own offset (attachPosition, a skeletal mesh's
// centre) adds no step.
//
// Spawns lit, mode 1, size 1, white, back-face culled; set everything else
// with updateRender3D.
//
// Every true/false here also takes 1/0, except the rotation slot beside a
// plain object, where a bare number is a yaw.
//
// An object to follow that is null fails the command without a report - in
// the position slot, [object, memoryPoint], [object, true] or [object,
// false], or the rotation slot - and so does a live one whose transform
// cannot be read at the call, except under [object, memoryPoint], where
// the mesh is made and follows the point once it can be read.
//
// Returns the khr_ handle, or '' after reporting the fault (or without a
// report, as above)
//
// ---- BOOL = updateRender3D ARRAY -------------------------------------------
// [handle, property, value] or [[handle, property, value], ...]. Update a
// persistent 3D mesh object: position | attachPosition | size | rotation |
// attachRotation | mesh | material | mode | sceneRead | effect | params |
// lit | twoSided | lodLock | inFront | showInFront | casterOnly | castShadow |
// receiveShadow | occluder | clothSimulation | physicsCollider |
// chainSimulation | simulationLod | textures | color | visible | blend |
// band | duration.
//
// The value forms (a property's own paragraph below adds to its form):
// size a number or [x, y, z] - multipliers of the mesh's own size | mode
// 0 (depth test), 1 (test and write) or 2 (overlay) | sceneRead true: the
// scene read through the mesh, tinted (colorGrade at its defaults), false:
// none | effect a builtin effect (addPostFX's list) or a .hlsl / .cube path
// | params up to 12 numbers, the effect's parameters in order (nil keeps
// the default) | lit true / false, or [ambient, diffuse] numbers (lit,
// with those; [ambient] alone sets the ambient) or [] (false) | color
// [r, g, b, a] | visible true / false | blend normal | additive | multiply
// | screen | lighten | darken (or 0 - 5; addPostFX's paragraph says what
// each does) | band [minDist, maxDist, falloff?] metres from the camera ([]
// clears) | duration seconds or [fadeIn, hold, fadeOut] ([] = permanent),
// counted from the update in real time - the renderer's session clock,
// which runs on while the game is paused or time-accelerated, as uiSleep
// counts and sleep does not; every duration below (addPostFX,
// addLocalPostFX, updatePostFX) counts the same way. A mesh is a builtin
// - box | steps | sphere | cylinder | cone | pyramid - a registry index or
// a .fbx path.
// Also taken, meaning the same: scale (size), lighting (lit), fxparams
// (params); in a material, the map albedo (diffuse), the params color
// (basecolor), metallic (metalness), gloss (glossiness), speccolor
// (specularcolor) and emissive (emissiveintensity), the routing input
// metalness (metallic), and the selector "" ("*"); in clothSimulation
// and chainSimulation, stretch (stiffness).
//
// castShadow and receiveShadow (both true by default) switch the mesh's
// shadows. castShadow false casts none - onto the world, under dynamic
// lights, onto our meshes and onto itself - whatever casterOnly says (an
// invisible casterOnly mesh then draws nothing at all). receiveShadow false
// takes none - not the world's, not our meshes' (its own included), not those
// our meshes cast under dynamic lights; the sun and every dynamic light still
// light it, and a face turned from the sun stays unlit. An unlit mesh (lit
// false) neither casts nor receives whatever these say. A shadow view
// distance of 5 m or less in the video options turns every mesh's shadows
// off, as if both were false on all of them, and idles the machinery only
// shadows use; raising it turns them back on. The setting is read about
// four times a second while any mesh or post-processing pass exists and 3D
// frames are drawn, so a change takes effect within about a quarter second.
//
// inFront true draws the mesh in the game's first-person view-model layer -
// the depth slice the weapon and hands occupy - so it stands in front of the
// whole world and still sorts correctly against the weapon and hands
// themselves; it casts and receives shadows as any mesh does, always draws
// its full-detail level, and is not drawn in a view with no view model
// (third person). Effect meshes ignore it.
//
// showInFront (true by default) false hides the mesh while the view is a
// first-person one - a frame in which the game draws its first-person weapon
// and hands layer, the layer inFront draws in - so a mesh on the player's own
// head or body does not fill the first-person camera. Measured on foot with a
// weapon; any other view counts by whether the game draws that layer in it
// (expected: none in third person, Zeus and scripted cameras, so the mesh
// shows; a first-person view of another unit draws that unit's, so it hides
// there too) - getRenderStats' firstPerson (1 / 0) shows the verdict while a
// visible mesh with showInFront false (inFront false) or a visible inFront
// mesh with showInFront true exists (0 when none does). The test is the main
// view's, whichever unit the mesh is on, and lags one frame at a switch of
// view; a mesh that appears, turns visible or takes
// showInFront false stays hidden until the first frame has been measured. A
// picture-in-picture (a scope, mirror or camera feed) still shows it. While
// hidden it casts nothing, as a mesh with visible false - unless casterOnly
// is true, which keeps its shadow while it is hidden; a declared occluder
// still hides what is behind it. With inFront true it is never drawn, exactly
// as with visible false.
//
// lodLock true draws the full-detail level at every distance.
// clothSimulation and chainSimulation each turn it on - what they simulate
// is level 0's alone - and it stays on when they end; lodLock false is
// refused while one of them is on. A skeletal binding leaves it as it is:
// every level carries its own bone weights, so a far character draws a
// coarser level, skinned as level 0 is.
//
// Our meshes hide one another automatically: a mesh stops being drawn while
// another of our meshes provably covers it completely. A mesh can hide what
// is behind it only when it is solid and opaque (blend normal, color alpha 1,
// no cutout, blend or .hlsl material, no effect or band, mode 0 or 1, not
// inFront), is solid around its centre (a closed part - watertight at every
// detail level - enclosing it; open extras such as decals, strips or open
// tubes are fine, but a centre inside nothing closed, like a ring's or a
// sheet's, or a solid with holes, cracks or T-junctions, does not qualify),
// and is drawn as imported -
// not skinned by a skeletal binding, not a clothSimulation or
// chainSimulation, no vertex stage. Each frame the cull takes at most 16
// occluders - those largest on screen - and skips any too small on screen to
// matter or that the camera is inside or right beside. A mesh reaching below
// the terrain hides with only the part of it that stands clear of the terrain
// around it, and not while the camera is lower than that part. Beyond 1000 m,
// the part of a mesh that terrain between it and the camera hides still hides
// what is behind it, which lies behind the same terrain. The engine's own
// scene is never hidden this way, and nothing ever hides the engine's scene.
// getRenderStats counts the occluders the cull drew (occluders) and the
// meshes in view it hid (occludedMeshes).
//
// occluder true makes the object's box - exactly its size, at its position
// and rotation - hide our meshes behind it whatever is drawn there: an
// invisible object becomes an invisible occluder, and a mesh the automatic
// rule turns down occludes with its whole box. It is a promise the script
// makes: a mesh behind the box stops being drawn even where the box covers
// nothing opaque, so give it only a box that is solid all the way through. A
// declared box counts at any size on screen and ranks ahead of the automatic
// occluders among the 16 a frame; one the camera is inside or right beside
// hides nothing that frame. A mesh is hidden only when it lies wholly behind
// the box as a coarse screen grid sees it - each face counted at the distance
// of the farthest corner of its part in front of the camera - so one only
// partly behind, or behind the nearer part of a box seen at a slant, stays
// drawn. occluder false (the default)
// leaves the object to the automatic rule.
//
// clothSimulation and physicsCollider each take true, false, [] or an array
// of [key, value] pairs. clothSimulation simulates the mesh: its geometry
// must be painted with the vertex groups or colour sets kh_cloth_sim (where
// it moves) and kh_cloth_pin (where it does not) - a gradient between them
// blends smoothly from rigid to free and is centred, so painting a vertex
// halfway lets it hang about halfway - and an unpainted mesh simply does not
// simulate. Keys: stiffness | bend | damping | drag | mass | gravity |
// maxStretch | thickness | friction | maxSpeed | teleport | range | sleep |
// shell | wind | buoyancy | waterDrag | selfCollision | selfThickness |
// selfFriction | iterations | substeps. Their ranges: stiffness, bend,
// damping, drag, thickness, friction, shell, waterDrag and selfFriction 0 -
// 1; mass 0.001 - 100; gravity -10 - 10; maxStretch 1 - 4; maxSpeed 0.1 -
// 1000; teleport 0.05 - 1000; range 0 - 200; sleep 0 - 10; wind 0 - 10;
// buoyancy 0 - 10; selfThickness 0.001 - 0.25; iterations 1 - 64;
// substeps 1 - 8.
//
// Cloth reacts to the world wind automatically: wind scales it per object (0
// ignores wind) and it acts through each face's own facing, so cloth edge-on
// to the wind is barely pushed. It also reacts to water - sea level is 0 ASL
// and anything below it is submerged - where buoyancy sets the upward
// acceleration as a multiple of standard gravity (9.81 m/s^2), whatever the
// object's own gravity: at the default gravity 1, 0 sinks as if in air, 1 is
// neutral and above 1 rises (at gravity g, buoyancy g is neutral) - waterDrag
// replaces drag while under, and wind stops acting.
//
// shell is for THICK cloth: a solidified mesh is two separate surfaces that
// share no geometry except the rim, so without it the front and back drift
// apart and can turn inside out. Set shell to a little more than the mesh's
// own thickness in metres to bind opposing surfaces together; leave it 0 for
// single-sided cloth.
//
// selfCollision (default false) stops the cloth passing through itself: with
// it true, no part of the cloth comes nearer to another part of the same
// cloth than selfThickness metres (default 0.01), from the side it came
// from, and the layers grip each other with selfFriction (0-1, default 0.3;
// the plain friction key is only for colliders). Layers the mesh was
// modelled closer than selfThickness keep their modelled gap, so a folded or
// thick mesh is not blown apart, and two points side by side along the
// surface closer than twice selfThickness at rest are kept at least half that
// rest distance apart. It prevents crossings; it does not pull a
// tangle out, but it does not hold one either: where the cloth already
// passes through itself it is left free to come apart. Thin, fast or
// coarsely tessellated cloth can still slip through in places - raise
// selfThickness - and a collider always wins over the cloth's own layers. A
// face or edge so large beside the mesh's mean edge that its bounding box
// covers more than 1024 cells of the self-collision grid (a cell is at least
// the mean edge) takes no part in it.
// When the object snaps rather than moves (a jump or turn under the teleport
// distance that no cloth could follow), self-collision stands down for one
// second so the cloth can fall open, then resumes.
//
// physicsCollider makes the mesh push cloth around, using its real
// triangles: keys margin (0 - 10) | friction (0 - 1) | inside (inside contains cloth instead
// of excluding it) | convex. A collider must be a closed, outward-facing
// mesh - inside and outside are decided from its winding.
//
// convex (default false) is the cheap path for a collider built from CONVEX
// pieces - a body made of a capsule per bone, say, all in one mesh, each
// piece its own closed shell that shares no vertex with another: cloth then
// takes each piece's distance as the largest of its faces' plane distances -
// exact on its faces, and at its edges and corners the standoff keeps the
// piece's sharp shape rather than rounding it - which costs about one plane
// test per particle instead of a search of the triangles, so many such
// colliders near many cloths stay cheap. A piece that is not convex is not
// detected and collides as those planes say: cloth is held off only where it
// is behind every one of the piece's face planes, which is less than the
// piece wherever it is concave, so cloth can pass into a concave part.
// Margin, friction, inside and the skeletal binding all work as before; a
// skinned convex collider refits only its planes per pose. Chains take it
// too.
//
// A skeletal binding (position [object, true]) collides as its skinned pose,
// rebuilt whenever the pose changes, so a closed low-poly body mesh bound to
// a character lets cloth collide with the character as it moves; its
// triangles are refitted into the collider on every pose change, so keep it
// coarse. Its limbs sweep across each frame in the cloth's substeps and
// carry cloth by friction as they swing, though a limb moving more than half
// its thickness in one substep can still pass through. A hidden collider
// (visible false) is not drawn and still collides, which is how an invisible
// collision body is made.
//
// Gravity, the air (still, or moving with the world wind), water and physics
// affectors (addPhysicsAffector) are the external forces. Cloth lands on the
// terrain, with its friction key: the ground itself, not what stands on it -
// buildings, floors, bridges and rocks need colliders, and cloth on an upper
// floor falls through it to the ground below.
//
// chainSimulation simulates the mesh's own skeleton as a chain of rigid
// bodies - a rope, a chain, an ammo belt: every bone from the start bone
// down swings, joined to its parent at its head, the bones drive the mesh,
// and what collides is the mesh's geometry, against the terrain and every
// physicsCollider. It takes true, false, [] or [key, value] pairs, and needs
// a model imported with a skeleton whose bones skin its vertices.
//
// Keys: start (a bone name; nil = the skeleton's root), which stays pinned
// where the object puts it, or under a skeletal binding (position [object,
// true]) where its memory point does | end (a bone name; nil = the chain's
// tip when it does not branch) | startPoint and endPoint ("head" or "tail",
// nil = the default), which point of each of those two bones the chain uses.
// A bone's head is its own origin and its tail is its far end - the next
// bone's head, or on a tip bone the reach of its own geometry. startPoint is
// where the chain hangs off the start bone (default "tail"; "head" hangs it
// one bone higher, so the mesh below is that much shorter), and endPoint is
// the point endPosition places and endRotation turns (default "head"; "tail"
// holds the very end of the last bone, which is what a rope's tip usually
// wants) | endPosition and endRotation, the end bone's hold, set up like the
// mesh's own position and rotation: nil leaves it free, true follows the end
// bone's memory point on a bound skeleton (else its place in the mesh),
// false holds it where it stood when the chain started, [x, y, zASL] or yaw
// / [pitch, yaw, roll] / [vectorDir, vectorUp] hold it there in the world,
// an object follows that object, and endPosition [object, memoryPoint]
// follows a memory point -
// which links two objects with one chain; endRotation true then follows that
// memory point's (or that object's) rotation. endPosition places the end
// bone's point - its head unless endPoint says "tail" - and an end held away
// from where it stands is brought there at 2 m/s rather than snapped |
// stiffness (0-1, 1 = the joints hold exactly) | bend and twist (0-1: 0
// limp, 1 rigid) | swingLimit and twistLimit (degrees, 180 = none - a chain
// does not collide with itself, and limits are what keep its links from
// folding through each other) | damping | linearDamping and angularDamping
// (0-1, default 0 = off: the share of each link's motion, and of its spin,
// relative to the object carrying the chain that is removed every 1/120 s -
// the loss damping cannot give a chain swinging as one) | drag (against the
// air, which moves with the world wind times wind) | mass (kg, the whole
// chain) | gravity |
// thickness | friction | maxSpeed | teleport | range | sleep | wind |
// buoyancy | waterDrag | iterations | substeps. A simulating chain is drawn
// at its authored size (size is kept for when it stops), forces lodLock,
// cannot also be clothSimulation, and does not itself push cloth. A chain
// that cannot simulate (no skeleton, or a named bone missing) is named once
// in the RPT. The ranges of a chain's numbers: stiffness, bend, twist,
// damping, linearDamping, angularDamping, drag, thickness, friction and
// waterDrag 0 - 1; swingLimit and twistLimit 0 - 180; mass 0.001 - 1000;
// gravity -10 - 10; maxSpeed 0.1 - 1000; teleport 0.05 - 1000; range 0 -
// 200; sleep 0 - 10; wind 0 - 10; buoyancy 0 - 10; iterations 1 - 16;
// substeps 1 - 8.
//
// simulationLod is the level of detail of the cloth and chain solvers, ON BY
// DEFAULT: an object that is both far from the camera AND small on screen
// runs a share of its own iterations and substeps (a small cloth close by,
// or a large one seen from afar, runs them whole; nothing is ever frozen or
// skipped, and self-collision, colliders and the sleep rule are untouched).
// It takes true (on, keeping its dials), false or [] (off - the object's
// dials run whole wherever it is), or [key, value] pairs (on, setting those
// keys): floor (0.05-1, default 0.25) is the least share it runs at, so the
// default is a quarter of the iterations and half of the substeps at the far
// end (each never under 1; 1 = off); fullDistance (m, default 25) - nearer
// than this the solver runs whole; minDistance (m, default 100) - beyond
// this, and below minScreenShare, it runs at the floor; fullScreenShare
// (default 0.06) - an object whose bounding sphere spans more of the screen
// height than this runs whole, however far; minScreenShare (default 0.015) -
// below this, and beyond minDistance, it runs at the floor. The defaults are
// fitted to a 1 x 2 m cape at the default field of view: whole out to 25 m,
// at the floor from 100 m, and the two shares are what such a cape spans at
// those distances, so a smaller cloth is reduced sooner and a larger one
// later. Between the two distances and the two shares it ramps smoothly,
// taking the better of the distance and the screen verdicts; iterations give
// first, substeps only below half share. fullDistance must be under
// minDistance and minScreenShare under fullScreenShare. getRenderStats
// reports simLodShare, the share of the solver work run last frame.
//
// position and rotation each take a game OBJECT to follow it every frame, or
// any ordinary value to detach and take that value. position also takes
// [object, memoryPoint] to follow a model memory point (bone); rotation then
// takes true/false to start or stop following that bone's rotation, which
// costs no engine call and leaves the attachment alone. position [object,
// true] is the skeletal binding (see addRender3D) - rotation's true/false
// then says whether the model turns with the object, and size is ignored
// until the binding ends; [object, false] is a plain attach to the object.
// While position follows a plain object (the object itself or [object,
// false]) rotation true follows that object's rotation and false stops it (a
// rotation that follows another object is left alone); while position
// follows nothing, true / false is refused.
// The two lanes are independent: a new position (or a detach) never changes
// a rotation that follows an object of its own - but for the position's own
// object (rotation true, or that object named), which a new position that
// follows an object (the object itself, a memory point or a skeletal
// binding) takes over as rotation true under it, and a detach ends. A
// rotation that follows the binding (its memory point, or the skeletal
// binding's object) moves to a new binding with it, and ends with any
// other position or a detach. A rotation that ends leaves the mesh at the
// rotation it holds.
//
// attachPosition [x, y, z] and attachRotation nil|yaw|[pitch, yaw, roll]|
// [vectorDir, vectorUp] offset the mesh WITHIN the frame it is attached to,
// in the followed object's own axes (x right, y forward, z up; the vectors
// too) rather than world axes - so a
// mesh can sit off the memory point or turned away from it. Each affects
// only its own lane and only while that lane follows an object; both are
// remembered across a detach, and nil clears.
//
// material params: basecolor | roughness | metalness | emissiveintensity |
// normalstrength | cutoff | alphamode opaque|cutout|blend (blend: texels
// with alpha >= 0.9 draw solid with depth, the rest as a post-scene
// translucent part - hardware alpha, no depth write, back-to-front; casting
// is per object, never per material).
//
// Texture animation (every shader kind; KH_UV_ANIM) - params that move every
// map of the submesh together, in colour, in the cutout / blend coverage and
// in the shadows it casts: uvScale (a number or [u, v]: repeats per uv unit,
// negative mirrors; default 1) | uvOffset [u, v] | uvScroll [u, v] (uv units
// per second; the picture appears to move the opposite way) | uvRotation
// (degrees, about the tile's centre) | uvRotationSpeed (degrees per second) |
// uvWarp [amplitude, frequency, speed] (a ripple across each axis: amplitude
// in uv units 0 - 1, frequency in ripples per uv unit, speed in cycles per
// second; the ripple stays on the surface while the picture scrolls through
// it) | flipbook [columns, rows, fps, frames?] (a sheet of frames, row by
// row from the top-left; frames defaults to columns x rows - a bilinear or
// mipmapped read at a cell's edge takes from its neighbour, so give a sheet a
// gutter or few mips). The lookup is scale x rotation about the tile's centre,
// then offset + scroll x time, then the warp, then the flipbook cell; the
// clock is the renderer's session clock - real time, which runs on while the
// game is paused or time-accelerated - so objects sharing a material move in
// step. Normal maps turn and stretch with the picture (a uniform tiling does
// not steepen them; the warp does not bend them). While a cutout or blend
// material whose alpha comes from a map
// animates with the clock and its object casts, the shadows are redrawn each
// frame as for a caster that keeps moving - the sun's maps and every live
// dynamic light's (all of them: each set is keyed as one) and the volume
// seam's; an opaque one costs its shadows nothing.
//
// A .hlsl material (the shader given as a .hlsl path; the USER SHADER
// CONTRACT in cb.hlsl is its full reference) takes every map, routing input
// and param of both builtin shaders: maps diffuse | normal | orm or as |
// emissive | specular or smdi | speccolor - orm and as are ONE slot, as are
// specular and smdi, so give one name of each pair, and the name says what
// the map holds (as and smdi as described for "arma" below, smdi read as
// data rather than colour) - routing inputs occlusion | roughness | metallic
// | alpha | gloss | specular, and the params above plus glossiness |
// specularcolor | fresnel. It also takes the texture slots user0 .. user5,
// which only its own code reads (KhUserTex): their third element is "srgb"
// for colour data or "linear" (the default) instead of a routing array. It
// also takes two params that act only when the .hlsl defines a vertex stage
// (KhUserVertex, which deforms the mesh on the GPU): vertexBound (metres, default 0) is the
// farthest the stage moves any vertex - culling, the level of detail and the
// shadow fits pad the object by it, so a value too small lets the mesh
// vanish or its shadow clip at the screen edge, and one too large costs only
// a looser fit - and vertexStatic (0 or 1, default 0) says the result
// depends on no clock and no camera, so it is worked out again only when the
// object moves, turns, resizes or changes colour or material; an object with
// a vertex stage is drawn on its own, outside the instanced batches.
//
// The shader "arma" is Arma's specular-gloss workflow on the same lighting:
// maps diffuse | normal | as (ambient occlusion in green) | smdi (green =
// specular, blue = gloss, black matte to white shiny) | speccolor (tints the
// specular) | emissive, routing inputs occlusion | gloss | specular | alpha,
// and params basecolor | glossiness (without an smdi map, default 0.2) |
// specularcolor [r, g, b] | fresnel [N, K] or an rvmat string with
// fresnel(N,K) in it (refractive index and absorption, default 1.5, 0) |
// emissiveintensity | normalstrength | cutoff | alphamode.
//
// Every true/false value also takes 1/0, except rotation and chain
// endRotation, where a bare number is a yaw.
//
// Faults are reported, except an object to follow that is null - in
// position or rotation (every form above that names one), a skeletal
// binding's parent when its mesh changes, or a chain's endPosition /
// endRotation (endPosition's [object, memoryPoint] too) - and a live one
// whose transform cannot be read at the call, for position (an object,
// [object, true] or [object, false]) and rotation (an object): that triple
// fails without a report. The batch form returns true only if every
// triple applied
//
// ---- STRING = addPostFX ARRAY ----------------------------------------------
// [effect, params?, [r,g,b,a]?, band?, blend?, affectUI?, duration?]. Create
// a persistent fullscreen post-processing pass. affectUI takes 'scene' |
// 'UI' | 'both' | true/false | 1/0.
//
// A size an effect's params give in pixels (blur, bloom and halation
// radius, grain size, aberration, distortion, lens-flare chroma,
// anamorphic length, glitch slice and split, clarity radius, deband range,
// the CRT's aberration, wobble and mask size, fog scatter's radius) is in pixels of a
// 1080-row picture and scales with the picture's height, so a look holds
// at any resolution - here, in addLocalPostFX and on effect meshes alike.
// The effects' own built-in pixel spans scale the same way (the outline's
// width, glitch rows, blocks and static, the rain lens's film blur and
// refraction, the sun flare's occlusion test). What stays per pixel:
// sharpen's 1-px neighbourhood, anti-aliasing, per-pixel dither and
// sampling noise, and SSGI's sample-grid widths (its radius is in metres).
//
// The builtin effects (by name, or by id 0 - 23 / 31 in this order) and
// their params with the defaults; colour = the pass's colour. solid (no
// effect: an ordinary mesh; meshes only) | invert: (1 - picture) x colour |
// colorGrade [saturation 1, contrast 1, brightness 1, gamma 1], tinted by
// colour | vignette [startRadius 0.5, softness 0.5], colour at the edge |
// chromatic [strengthPx 3] | grain [amount 0.06, fps 24, grainSizePx 1.5,
// lumaResponse 0.7, chroma 0.3] | sharpen [strength 0.5] | blur [radiusPx
// 2] | bloom [threshold 1, intensity 0.8, radiusPx 2] | distortion
// [amplitudePx 6, frequency 6, speed 2] | outline [depthEdge 4, lumEdge 2,
// sceneDarken 0.25, glowBoost 1.5], colour = the edge | pulse [x 0, y 0,
// zASL 0, radius 50, bandWidth 3, intensity 2] | halation [threshold 1,
// intensity 1.2, radiusPx 5] | fog [startDist 200, endDist 1200,
// skyAmount 1], colour = the fog | lensflare [threshold 1.1, intensity
// 0.7, ghosts 5, spacing 0.35, haloRadius 0.45, haloIntensity 0.6,
// chromaPx 3] | anamorphic [threshold 1.2, intensity 1.2, lengthPx 220,
// falloffPow 2, vertical 0] | sunflare [dirX 0, dirY 0.5, dirZ 0.8 (SQF
// axes), size 0.15, ghostDots 4, ringIntensity 0.6, starburst 1.2] |
// glitch [intensity 1, speed 8, sliceAmountPx 24, sliceBands 12,
// colorSplitPx 6, blockAmount 0.5, noiseAmount 0.35, burstiness 0.7] |
// clarity [strength 0.35, radiusPx 22] | deband [threshold 1.6, rangePx
// 16, grain 0.6] (threshold and grain in 1/255) | rainlens or rain
// [intensity 0.6, speed 1, condensation 0.35, refract 1] | crt [curvature
// 0.12, scanlines 0.45, lineCount 540, maskStrength 0.35, aberrationPx 1.5,
// flicker 0.25, rollingBand 0.3, cornerRadius 0.06, lineScroll 4 (lines
// a second), wobblePx 1.5, maskSizePx 3] | ssgi [intensity 0.7, radius 4
// (m), samples 12 (4 - 32), bias 0.15 (0 - 0.9), falloff 2 (the bounce's
// falloff exponent over the radius, at least 0.25), saturation 1, range
// 300 (m; fades from 70 %), fireflyClamp 3 (0 = none),
// albedoTint 0.65, giOnly 0 (1 = the light alone), offsetScale 1 (0 = 1),
// resolveWidth 0 (0 = 3; 1 - 8)] | fogscatter
// [intensity 1, radiusPx 0 (0 = from the picture's height; 2 - 96), taps
// 12 (4 - 24)] | dynamicLightFog (below). A .cube LUT takes [strength 1,
// encode 0 (0 = by lane, 1 = never, 2 = always)]. ssgi, fogscatter and
// dynamicLightFog draw nothing on the UI.
//
// The blend modes, by the colour's alpha (the opacity): normal mixes;
// additive adds colour x opacity; multiply multiplies the picture by
// lerp(1, colour, opacity); screen is 1 - (1 - a)(1 - b), by opacity;
// lighten and darken take the larger / smaller of the two, by opacity
// (an inFront mesh, a blend material's translucent texels, or any frame the
// scene cannot be copied: lighten takes the larger of the picture and
// colour x opacity, darken acts at opacity 1 only).
// multiply and screen are the display formulas, taken as they stand: on
// the scene (linear HDR, past 1 where it is bright) a colour past 1
// brightens under multiply, and under screen a source or a background
// past 1 comes out darker, even below 0 - give them display-range
// colours there. A mesh with colour alpha below 1 on blend normal is mixed
// into the scene in a display-like space (x / (1 + x): a bright sky
// behind grey glass stays bright) and written opaque; an inFront mesh, a
// blend material's translucent texels, an effect mesh on normal blend (and
// any mesh on a frame the scene cannot be copied) mix in linear HDR, as the
// game's own glass does (KH_TRANSL_ROUTES: all kept as they look).
//
// Every builtin effect runs one formula on the scene (linear HDR, before
// the game's tonemap) and on the UI (the finished display frame, 0 - 1).
// invert, colorGrade (contrast turns about 0.5), grain and deband (steps
// of 1/255) are written for display values, so on the scene they act on
// linear light: tune them there as they look there. A .cube LUT is looked
// up over its [0, 1] domain - a value past 1 takes the LUT's edge, the
// .cube rule for input outside the domain - and on the scene through a 2.2
// encode / decode; a DOMAIN_MIN / DOMAIN_MAX other than 0 / 1 is resampled
// onto [0, 1], so an HDR-domain LUT's part past 1 is not used. A .cube is
// read and parsed on the render thread when its pass first draws, and
// again after a device reset (alt-tab in exclusive fullscreen, a
// video-settings change): a large one (65 or more points a side) holds
// that frame for a moment. A lattice value that is not a number, or lies
// beyond +-65504, refuses the LUT with a report.
//
// The glows and blurs - blur, bloom, halation, lens flare, clarity, the
// rain lens's condensation film and fog scatter - read a pre-filtered copy
// of the picture (a pyramid of halved levels made for each pass from its
// own source, thresholded first for bloom, halation and lens flare), each
// tap averaged over (a little past) the gap to its
// neighbours, so a large radius spreads smoothly instead of showing copies
// of the scene at its tap spacing, a lens-flare ghost is soft instead of
// blocky, and fog scatter carries no grain. The anamorphic streak reads a
// copy halved along its own axis only (thresholded too), so it runs as one
// continuous line instead of a row of dots and stays one pixel thin. Taps a
// pixel apart or less read the picture itself, as before - except lens
// flare's: its ghosts and halo always read the copy, over two pixels at
// least. The same holds in addLocalPostFX, on effect meshes, over the UI and
// in a picture-in-picture.
//
// dynamicLightFog (KH_DLF) lights the game's fog with its dynamic lights -
// street lamps, flashlights, fires, vehicle lights: each glows through the
// fog around it in its own colour and brightness, a spot lighting its cone
// alone into a beam, as bright as the light and as thick as the fog there.
// The density is the game's own fog as it stands, thinning with height as
// the game's does, so with the game's fog at 0 the pass adds nothing unless
// baseDensity is set. The fog in front of it dims the glow as the game's fog
// dims anything (past the game's fog end, nothing shows), and the glow stops
// at the first surface - against the sky it reaches maxDistance. Params
// [intensity 1, fogScale 1, baseDensity 0, anisotropy 0.6, steps 16,
// maxDistance 250, shadows 2, maxLights 24]: intensity scales the light the
// fog scatters; fogScale multiplies the game's fog density and baseDensity
// (per metre) adds an even haze of its own; anisotropy (-0.9 .. 0.9) is how
// strongly the fog scatters forward - toward 0.9 the glow gathers where you
// look into the light, 0 spreads it evenly; steps (4 - 64) are the samples
// taken through each light's reach, gathered toward its bright core, fewer
// being cheaper and grainier; maxDistance (10 - 5000 m) is how far from the
// camera the fog is marched and lights are taken; shadows 0 casts none, 1
// lets our own meshes cut the light into shafts (with allowDynamicShadows
// true, which draws the light maps they cast from), 2 adds the game's own
// geometry as the camera sees it - a wall, a fence, a building corner
// between a light and the fog; maxLights (1 - 64) keeps the lights whose
// reach comes nearest the camera. The colour tints the fog's light; its
// alpha is the pass's opacity. The fog is worked out on the grid
// setSsgiScale sets (half size by default) and filtered up to the picture,
// so geometry thinner than a grid texel - a wire, a thin post - can show
// the glow of what lies behind it (setSsgiScale 1 works it out per pixel);
// getRenderStats reports the grid work's GPU time as gDlFogUs (the pass's
// last step, the upsample onto the picture, counts in gFlush). The game's
// geometry casts only what the camera sees of it: a wall off screen or
// behind the camera casts no shaft, a thin post or branch seen side-on
// casts a faint one, something standing within about a metre of the
// surface behind it (more, farther off) is taken as part of that surface
// (a lamp between the two is shadowed by it), and a surface within a metre
// of a light - its housing, the hand or vehicle holding it - casts none, as
// the game's lights shine through their own. The lights are the ones the
// game uploads for the frame, as our meshes' dynamic lighting has them, and
// a visible pass keeps that capture running as a lit mesh does; with FSAA
// at 1x none are read and the pass adds nothing. addLocalPostFX confines
// it as it confines any pass, by the surface each pixel shows: where that
// surface lies outside the volume - the sky included - the glow is cut,
// though the fog it crosses may lie inside (with inverse, the reverse). On
// a mesh (updateRender3D's effect), in a picture-in-picture and on the UI
// it draws nothing. Its cost grows as lights x steps x the grid's texels,
// shadows adding to every step: the maxima together - 64 lights reaching
// into the view, 64 steps, setSsgiScale 2, shadows 2 - are estimated (not
// measured) at a second or more a frame on a mid-range GPU - in one
// draw, and a draw that runs about two seconds makes Windows reset the
// driver, which the game does not survive. Raise them a step at a time,
// watching gDlFogUs.
//
// Returns the khr_ handle, or '' after reporting the fault
//
// ---- STRING = addLocalPostFX ARRAY -----------------------------------------
// [[x,y,zASL], radius, falloff, effect, params?, [r,g,b,a]?, shape?, blend?,
// duration?, inverse?]. Create a persistent post-processing effect confined
// to a world-space volume (inverse = true or 1: everywhere except it).
// The volume confines every blend mode alike: where the surface a pixel
// shows lies wholly outside it, the picture is left exactly as the pass
// found it - as outside a band, and outside the UI's coverage on the UI.
// A volume wholly beyond one of the view's four edges (the edge planes
// run on behind the camera) is normally skipped at no cost; any other
// costs a whole-screen pass however small it appears, and an inverse one
// always does.
//
// Returns the khr_ handle, or '' after reporting the fault
//
// ---- BOOL = updatePostFX ARRAY ---------------------------------------------
// [handle, property, value] or [[handle, property, value], ...]. Update a
// fullscreen / local post-processing pass: position | effect | params | ui |
// uiSpill | radius | falloff | localSphere | shape | inverse | textures |
// color | visible | blend | band | duration.
//
// textures (here and in updateRender3D, for an effect mesh) gives a .hlsl
// effect textures of its own: [[path, slot, colour?], ...] with slot
// "user0" .. "user5" and colour "srgb" or "linear" (the default) - png,
// jpg, jpeg, tga, bmp, dds, paa or pac, found as the .hlsl is (Documents\Arma
// 3\kh_framework\rendering, then every mod's 'rendering' folder). The array
// is the whole set: a slot it does not name is cleared, and [] clears them
// all. The effect's own code reads them (KhUserTex, cb.hlsl's USER SHADER
// CONTRACT). A builtin pass carries them too: they are bound for it on
// every route (the scene chain - its side draws included - effect meshes,
// the UI lane, a picture-in-picture) for a builtin effect written to read
// them. No builtin effect reads them yet: on one they cost their loading
// and binding each draw, and the pass is not fused into the pass before it
// (a pass after it may still fuse into it).
//
// uiSpill true lets a .hlsl effect on the UI spill past the UI's coverage
// (bloom, halation, lensflare and anamorphic always do, the other builtins
// never: on them it changes nothing). shape sphere | ellipsoid (0) or cube
// | box (1); localSphere [radius, falloff?] ([] = not localized); radius a
// number or [x, y, z] metres; falloff metres.
//
// Every true/false value also takes 1/0.
//
// Faults are reported; the batch form returns true only if every triple
// applied
//
// ---- STRING = addPhysicsAffector ARRAY -------------------------------------
// [type, [lifetime, position, vectorDir, scale, ...]]. Create a physics
// affector that pushes simulated cloth (clothSimulation). type = wind |
// turbulence | vortex | force | radial | drag. The second array is lifetime,
// position, then vectorDir ONLY for the types that have a direction (wind,
// vortex, force), then scale, then the type's own values with falloff last;
// any slot but position may be nil for its default, and trailing slots may
// be left off.
//
// lifetime = seconds or [fadeIn, hold, fadeOut] (nil, [] or 0 = permanent);
// an affector deletes itself when its lifetime ends. It counts real time,
// as a duration does (updateRender3D): on through a pause or time
// acceleration.
//
// position = [x, y, zASL].
//
// vectorDir = [x, y, z] in world axes (default north; vortex default up).
//
// scale = a radius in metres or [x, y, z] half-extents along the affector's
// right, forward (vectorDir) and up - world east, north and up for a type
// with no direction - and nil or 0 acts everywhere.
//
// falloff = the outer fraction (0-1) of that region over which the effect
// fades to nothing.
//
// The ranges of the type's own numbers: wind speed -200 - 200; turbulence
// strength 0 - 200 (a negative one is 0: no turbulence), eddySize 0.01 -
// 1000, rate 0 - 50; vortex swirl, inflow and updraft -200 - 200, core 0
// - 1000; force and radial strength -1000 - 1000; drag strength 0 - 1. A
// radius or half-extent is held to 100000 m; a negative lifetime (or part
// of one) counts as 0, and a radius of 0 or below acts everywhere.
//
// wind [lifetime, position, vectorDir, scale, speed 10, falloff 0.25]: air
// blowing along vectorDir at speed m/s.
//
// turbulence [lifetime, position, scale, strength 5, eddySize 2, rate 0.5,
// falloff 0.25]: swirling gusts - strength is the typical (RMS) air speed
// (m/s), eddySize the size of a swirl (m), rate how fast they change (per
// second).
//
// vortex [lifetime, position, vectorDir, scale, swirl 10, inflow 0, updraft
// 0, core 0, falloff 0.25]: air turning about the axis vectorDir,
// counter-clockwise seen from its tip for a positive swirl, which is the
// speed (m/s) at the core radius - still on the axis, falling away beyond
// the core; inflow draws toward the axis and updraft lifts along it (m/s,
// negative reverses); core = the core radius in metres (0 = a quarter of the
// scale across the axis, or 1 m when unbounded).
//
// force [lifetime, position, vectorDir, scale, strength 9.80665, falloff
// 0.25]: acceleration along vectorDir (m/s^2; 9.80665 is one g).
//
// radial [lifetime, position, scale, strength 20, falloff 1]: acceleration
// away from position (m/s^2; negative pulls toward it), fading to nothing
// within 0.1 m of it.
//
// drag [lifetime, position, scale, strength 0.5, falloff 0.25]: thicker air
// - the fraction of the cloth's velocity removed per 1/120 s (0-1), on top
// of the cloth's own drag.
//
// wind, turbulence and vortex move the AIR: they add to the world wind and
// push cloth exactly as it does - through each face's own facing, so cloth
// edge-on to them is barely pushed, less for heavier cloth, not under water,
// and the cloth's wind key does not scale them. force and radial push every
// simulated point alike, like gravity. A cloth takes every affector in
// reach. physicsAffectors in getRenderStats counts the live ones.
//
// Update with updatePhysicsAffector; delete with removeRenderHandler.
//
// Returns the khpa_ handle, or '' after reporting the fault
//
// ---- BOOL = updatePhysicsAffector ARRAY ------------------------------------
// [handle, property, value] or [[handle, property, value], ...]. Update a
// physics affector (addPhysicsAffector): lifetime (counts from now) |
// position | rotation (vectorDir; wind, vortex and force only) | scale |
// params (the whole parameter array as addPhysicsAffector takes it; the
// lifetime counts from now) | any of the type's own values by name - wind:
// speed | falloff; turbulence: strength | eddySize | rate | falloff; vortex:
// swirl | inflow | updraft | core | falloff; force, radial and drag:
// strength | falloff. nil sets a value back to its default. The type cannot
// change - remove the affector and add another.
//
// Faults are reported; the batch form returns true only if every triple
// applied
//
// ---- BOOL = removeRenderHandler STRING -------------------------------------
// Remove a retained render object (mesh or post-processing pass) by its khr_
// handle, or a physics affector by its khpa_ handle. Returns true when
// something was removed; '' and a handle nothing owns do nothing and return
// false without a report (removeAllRenderHandlers clears everything)
//
// ---- BOOL = removeAllRenderHandlers ----------------------------------------
// Remove every retained render object (mesh and post-processing pass) and
// every physics affector. Returns true
//
// ---- ARRAY = allRenderHandlers ---------------------------------------------
// Every handle removeRenderHandler accepts right now: the khr_ handles
// (meshes and passes) in creation order, then the khpa_ handles (live
// physics affectors) in creation order
//
// ---- ARRAY = getRenderStats ------------------------------------------------
// Render statistics: counters and hook state, the render-thread frame trace
// and the dynamic-light state. The first call arms collection (statsArmed 0)
// and reports zero counters; later calls report what accumulated since.
// Nothing is timed or counted while collection is off. Timings: "<zone>Us" /
// "<zone>N" are the last frame's (the one-in-16 sampled zones are noisy there);
// profWindow and gpuWindow are every zone's mean and worst frame since the
// arming; funnel counts the constant-buffer capture's work; gpuPasses and
// gpuPassWindow time each post-processing pass on the GPU (the first 32
// drawn in a frame; any past them go untimed). The result is an array of
// [name, value] pairs
//
// ---- BOOL = resetRenderStats -----------------------------------------------
// Zero the render counters and disarm collection until the next
// getRenderStats. Returns true
//
// ---- BOOL = setRenderAmbientOcclusion ARRAY --------------------------------
// [strength] or [strength, radius]. Screen-space ambient occlusion on our
// meshes only: strength 0 - 4 (default 1; 0 = off), radius 0.05 - 5 m
// (default 0.5)
//
// ---- BOOL = setSsgiScale SCALAR --------------------------------------------
// SSGI gather resolution multiplier: 1 full res, 0.5 half (default), 2
// supersampled; 0.25 - 2
//
// ---- BOOL = allowDynamicShadows BOOL ---------------------------------------
// Whether dynamic lights cast shadows from our meshes (false by default,
// and again at every mission start). The maps take video memory: 4 MB per
// shadow face (a light needs one to six), allotted six faces at a time up
// to 48 (eight lights, 192 MB), in one array that only grows - freed when
// a mission ends or starts or the device resets, not when the lights go
// or this is set false
//
// ---- BOOL = flushUIRender --------------------------------------------------
// Renders all UI-affecting passes, and turns on the UI driver that
// otherwise calls it each frame. Returns true when passes were queued
// ===========================================================================

// KH_PLAYER_ONLY (the reference above): false = this machine has no graphics
// and nothing of this header may start. Tested where anything starts - the
// Draw3D and UI drivers, a new object or affector - and at the mission reset.
// framework.hpp's g_is_player: hasInterface, written once, on the game thread,
// in pre_start (main.cpp); every caller is on the game thread.
inline bool kh_render_on() { return ::g_is_player; }

// KH_SESSION_RESET - an object back to what a fresh process holds: destroyed
// and value-initialised in place (zero, then its members' own initialisers),
// which is how its static storage began; arrays element by element. Only on an
// object no other thread can be touching (the session teardown's park).
template <class T> inline void kh_reinit(T& khri_x) { khri_x.~T(); ::new (static_cast<void*>(&khri_x)) T(); }
template <class T, size_t N> inline void kh_reinit(T (&khri_x)[N]) { for (T& khri_e : khri_x) kh_reinit(khri_e); }

// KH_STATS_ARMED (hoisted here for KH_PROF, whose scopes precede the stats
// block): collection is off until the first getRenderStats.
static std::atomic<bool> g_stats_armed{ false };
inline bool kh_stats_on() { return g_stats_armed.load(std::memory_order_relaxed); }
// KH_PROF: opt-in frame-time instrumentation, CPU and GPU, per main-depth
// cycle. Nothing here runs unless the stats are armed (a getRenderStats):
// a disarmed session pays one relaxed load per scope, no clock read, no query
// (the first fold after a disarm closes, unread, the bracket the armed cycle
// opened, and zeroes the armed-only accumulators once - KH_PROF_DISARM).
//
// CPU zones: KhProfScope takes two TSC reads (__rdtsc) around a region and
// adds the ticks and one entry to the zone's accumulators - relaxed atomics,
// because zones run on the render thread, the game thread under the park, the
// UI-phase thread and the worker pool. The fold at the main depth clear
// (kh_prof_fold) converts the accumulators to microseconds (the TSC's rate it
// measures against QPC between armed folds, KH_PROF_SUB; kh_prof_now /
// kh_prof_add intervals stay on QPC) and publishes them with the entry counts
// as "the ended cycle's" values. KH_PROF_TSC_CLOCK: a QPC call (~40 ns) is a
// zone's floor per timed call, as large as the work of the small hooks, and
// x16 on a sampled zone; a TSC read is a few ns. The TSC zones publish whole
// armed cycles only (the cycle the arming splits reads 0; by the next one
// the rate is measured).
// Worker-pool zones (cloth, chain, skin, BVH) sum across workers, so they
// are worker-microseconds and can exceed the frame; divide by the count. The
// BVH build's zone also takes the game thread's builds (a rigid collider's
// first use - kh_physics_bvh_for), a stall rather than worker time.
//
// GPU zones: one TIMESTAMP_DISJOINT query brackets each cycle (begun and
// ended at the main depth clear, on the render thread), and each zone owns
// a begin/end TIMESTAMP pair per ring slot. KhGpuScope issues End(begin)
// and End(end) on the context it is given - the immediate context, whether
// the render thread or the parked game thread holds it. The fold reads the
// slot KH_GPU_LAG cycles back with DONOTFLUSH and keeps the last value when a
// result is not ready, so no readback ever stalls the pipeline. Queries are
// created lazily on the first armed use and released with the resources.
// A zone that ran more than once in a cycle reports its last run (a pair
// holds one interval); the CPU count tells how many runs there were.
enum KhProfZone : uint32_t {
    // Render-thread hooks.
    KHP_HOOK_CLEAR, KHP_HOOK_DRAW, KHP_HOOK_MAP, KHP_UPLOAD_SCAN,
    // Injection and its systems.
    KHP_INJECT, KHP_ATTACH_STEP, KHP_SCENE_SYNC, KHP_OBJBUF_SYNC, KHP_SUN_LADDER,
    KHP_DLS_FRAME, KHP_DLS_WORLD, KHP_MASK_CAST, KHP_SVS_PRIME, KHP_SVS_VOL_COPY,
    KHP_VMIR, KHP_SSAO, KHP_CULL_BUILD, KHP_VIS_OCC, KHP_INST_PLAN, KHP_SNAPSHOT,
    KHP_PIP, KHP_BAND_COMMIT, KHP_THM_AUTOBUILD, KHP_DL_ACQUIRE,
    // The flushes and their systems.
    KHP_FLUSH, KHP_FLUSH_FRAME, KHP_FLUSH_UI, KHP_CLOTH_UPLOAD, KHP_SKIN_UPLOAD, KHP_SKIN_SYNC,
    // KH_PARK_WAIT: the graphics-lock acquire alone, per park (the game thread
    // stalled until the render thread reaches the engine's grant point), for
    // the Draw3D park and the UI driver's park; flushUiFrame is the UI park whole.
    KHP_FLUSH_LOCK_WAIT, KHP_FLUSH_UI_LOCK_WAIT, KHP_FLUSH_UI_FRAME,
    // flush_frame's pre-lock work, in three pieces: the attachment reaps and
    // the Draw3D sampler; the demand census under g_draw_list_mutex; the rest
    // up to the park decision (proxy step, chain prepare, the driver re-hoist,
    // the hook ensure).
    KHP_FLUSH_PRE, KHP_FLUSH_CENSUS, KHP_FLUSH_PREP,
    KHP_CHAIN_PREPARE, KHP_CLOTH_PROXY, KHP_PHYSICS_GATHER, KHP_MESH_GC, KHP_SCENE_CAPTURE,
    KHP_UI_MASK_DRAW,
    // Worker pool (worker-microseconds).
    KHP_W_CLOTH_SIM, KHP_W_CHAIN_SIM, KHP_W_SKIN_JOB, KHP_W_BVH_BUILD, KHP_W_BVH_REFIT,
    // One-shot builders (import time).
    KHP_LOD_BUILD, KHP_ENSURE_RES,
    // KH_PROF_HOOKS: every other installed context hook's own work (the swap chain's rare ResizeBuffers has no
    // zone), the driver's call left outside (hookUpdSub alone wraps its driver call: drvUpdSub below takes it out).
    // hookUpdSub / hookPsSrv / hookOmBlend / hookOmDss / drawRecord / hookCopyRegion / hookMisc are sampled
    // (KH_PROF_SAMPLE); hookOmRt / hookOmRtUav / hookCopyRes / hookResolve are
    // called far less, some calls with heavy work (a PIP leave, the flush at the resolve), and are timed whole, as is
    // drawPost (KH_PROF_TSC: its work - the view-model mirror's re-issued draw, the UI quad, the bracket - lands on
    // one draw or two a frame). The zones nest and are not additive: they time every call through the hook, ours
    // too, so the work our own code issues inside a zone (the flush inside hookResolve, the PIP leave inside hookOmRt
    // / hookCopyRes, our draws inside a draw step) also counts in the zones of the hooks it calls.
    KHP_HOOK_UPDSUB, KHP_HOOK_PSSRV, KHP_HOOK_OMRT, KHP_HOOK_OMRTUAV, KHP_HOOK_OMBLEND, KHP_HOOK_OMDSS,
    KHP_HOOK_COPYRES, KHP_HOOK_RESOLVE, KHP_HOOK_COPYREGION, KHP_HOOK_MISC, KHP_DRAW_RECORD, KHP_DRAW_POST,
    // KH_PROF_DRV: the driver's own Map / Unmap (inside hookMap) and UpdateSubresource (inside hookUpdSub), timed on
    // one call in KH_PROF_SAMPLE_EVERY of their own (kh_prof_drv_t0) with the TSC and scaled; their N is those calls
    // x KH_PROF_SAMPLE_EVERY (an estimate). Our share of hookMap is about hookMap - drvMap - drvUnmap.
    KHP_DRV_MAP, KHP_DRV_UNMAP, KHP_DRV_UPDSUB,
    // KH_PROF_UPLOAD: the funnel's steps for each captured upload (KhProfSub; one decision per upload, timed one in
    // KH_PROF_SAMPLE_EVERY): the capture (the copy), then each scanner, then the replay's image note.
    KHP_UP_CAPTURE, KHP_SC_PROJ, KHP_SC_LIVE, KHP_SC_LOCATOR, KHP_SC_PIP, KHP_SC_ENGCAM, KHP_SC_REG, KHP_SC_VIEW,
    KHP_SC_B2, KHP_SC_RPIMG,
    // KH_PROF_DRAWSTEP: reorder_pre_draw's steps for the engine's draws (KhProfSub; one decision per draw, our own
    // draws take none): dsShadowNote / dsMaskNote / dlSample sampled; timed whole the ones whose work lands on a few
    // draws a frame (dsSeamPump, dsPip, dsInfront: the seam / svs, PIP and in-front injections), or one draw in 8 or
    // 16, or a retry (dsCascbind, dsSkybind, dsLatch). mask cast, band commit, the UI mask and the injection keep
    // their own zones above.
    KHP_DS_SEAM, KHP_DS_PIP, KHP_DS_SHADOW, KHP_DS_MASK, KHP_DS_DL, KHP_DS_INFRONT, KHP_DS_CASCBIND,
    KHP_DS_SKYBIND, KHP_DS_LATCH,
    KHP_ZONE_N
};
static const char* const g_prof_zone_name[] = {
    "hookClear", "hookDraw", "hookMap", "uploadScan",
    "inject", "attachStep", "sceneSync", "objbufSync", "sunLadder",
    "dlsFrame", "dlsWorld", "maskCast", "svsPrime", "svsVolCopy",
    "vmir", "ssao", "cullBuild", "visOcc", "instPlan", "snapshot",
    "pip", "bandCommit", "thmAutobuild", "dlAcquire",
    "flush", "flushFrame", "flushUi", "clothUpload", "skinUpload", "skinSync",
    "flushLockWait", "flushUiLockWait", "flushUiFrame",
    "flushPre", "flushCensus", "flushPrep",
    "chainPrepare", "clothProxy", "physicsGather", "meshGc", "sceneCapture",
    "uiMaskDraw",
    "wClothSim", "wChainSim", "wSkinJob", "wBvhBuild", "wBvhRefit",
    "lodBuild", "ensureRes",
    "hookUpdSub", "hookPsSrv", "hookOmRt", "hookOmRtUav", "hookOmBlend", "hookOmDss",
    "hookCopyRes", "hookResolve", "hookCopyRegion", "hookMisc", "drawRecord", "drawPost",
    "drvMap", "drvUnmap", "drvUpdSub",
    "upCapture", "scProj", "scShadowLive", "scLocator", "scPip", "scEngcam", "scShadowReg", "scShadowView",
    "scB2", "scRpImg",
    "dsSeamPump", "dsPip", "dsShadowNote", "dsMaskNote", "dlSample", "dsInfront", "dsCascbind",
    "dsSkybind", "dsLatch",
};
static_assert(sizeof(g_prof_zone_name) / sizeof(g_prof_zone_name[0]) == KHP_ZONE_N, "KH_PROF: a name per zone");
static std::atomic<uint64_t> g_prof_ticks[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_count[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_us_pub[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_n_pub[KHP_ZONE_N];
// KH_PROF_DISARM: resetRenderStats' count (reset_stat_counters bumps it) and the one the last fold saw (render
// thread). A fold that finds collection off, or a reset since the fold before it, publishes zeros - the CPU zones
// and the GPU times alike - and reads no GPU slot: what the accumulators and the query ring hold was measured before
// the reset, and the first getRenderStats after one reports zero. The GPU times are zeroed by the fold itself, not
// only by the reset: a harvest that raced a reset made without the graphics lock (resetRenderStats' fallback) can
// store pre-reset times after the reset zeroed them, and no disarmed fold harvests again.
static std::atomic<uint32_t> g_prof_epoch{ 0 };
static uint32_t g_prof_epoch_seen = 0;
inline uint64_t kh_prof_freq() {
    static const uint64_t khpf = [] { LARGE_INTEGER khpf_f = {}; QueryPerformanceFrequency(&khpf_f); return khpf_f.QuadPart > 0 ? static_cast<uint64_t>(khpf_f.QuadPart) : 1ull; }();
    return khpf;
}
// KH_PROF_SAMPLE: the zones entered tens of thousands of times a frame
// (KHP_HOOK_MAP on every Map and Unmap, KHP_UPLOAD_SCAN on every scanned
// upload, and the hooks KH_PROF_HOOKS adds: kh_prof_sampled's set) take one
// clock pair in KH_PROF_SAMPLE_EVERY entries and scale the
// ticks; the count stays exact. Two QPC calls on ~50,000 entries a frame were
// ~4 ms of the armed hook figures and most of the armed-vs-unarmed frame gap.
static constexpr uint32_t KH_PROF_SAMPLE_EVERY = 16u;
static_assert((KH_PROF_SAMPLE_EVERY & (KH_PROF_SAMPLE_EVERY - 1u)) == 0u, "KH_PROF_SAMPLE: a power of two");
static std::atomic<uint32_t> g_prof_sample_ctr[KHP_ZONE_N];   // hooked_map's scope opens before its thread gate: any thread.
// KH_PROF_TSC_CLOCK: the zones' TSC ticks (KhProfScope, KH_PROF_TSC, KH_PROF_DRV with atomic adds, any thread;
// KhProfSub with plain adds, render thread - no zone is written both ways), converted by the fold (KH_PROF_SUB).
static std::atomic<uint64_t> g_prof_tsc[KHP_ZONE_N];
// KH_PROF_HOOKS: the hooks the engine calls thousands of times a frame join the sampled set.
inline bool kh_prof_sampled(KhProfZone khpz_z) {
    switch (khpz_z) {
    case KHP_HOOK_MAP: case KHP_UPLOAD_SCAN: case KHP_HOOK_UPDSUB: case KHP_HOOK_PSSRV: case KHP_HOOK_OMBLEND:
    case KHP_HOOK_OMDSS: case KHP_HOOK_COPYREGION: case KHP_HOOK_MISC: case KHP_DRAW_RECORD:
        return true;
    default:
        return false;
    }
}
// KH_PROF_SAMPLE_HASH: which calls are timed - a mix of the zone's call counter, not the counter's own residue. The
// residue locks onto call patterns: hookMap's Map and Unmap strictly alternate on one counter, so every timed call
// was an Unmap (or every one a Map). khpm_salt draws further decisions from the same count (the driver's timing,
// KH_PROF_DRV) independent of the first, so one's cost lands in the other's samples at its true rate.
inline uint32_t kh_prof_mix(uint32_t khpm_x, uint32_t khpm_salt) {
    khpm_x ^= khpm_salt * 0x9E3779B9u;
    khpm_x ^= khpm_x >> 16; khpm_x *= 0x7FEB352Du;
    khpm_x ^= khpm_x >> 15; khpm_x *= 0x846CA68Bu;
    khpm_x ^= khpm_x >> 16;
    return khpm_x;
}
inline bool kh_prof_pick(uint32_t khpk_c, uint32_t khpk_salt) {
    return (kh_prof_mix(khpk_c, khpk_salt) & (KH_PROF_SAMPLE_EVERY - 1u)) == 0u;
}
struct KhProfScope {
    KhProfZone khps_z;
    uint64_t   khps_t0;   // KH_PROF_TSC_CLOCK: the TSC at the entry.
    bool       khps_on;
    bool       khps_timed;
    uint32_t   khps_c;   // KH_PROF_SAMPLE_HASH: a sampled zone's call count (0 = none drawn).
    explicit KhProfScope(KhProfZone khps_zone)
        : khps_z(khps_zone), khps_t0(0), khps_on(kh_stats_on()), khps_timed(false), khps_c(0) {
        if (!khps_on) return;
        if (kh_prof_sampled(khps_zone)) {
            khps_c = g_prof_sample_ctr[khps_zone].fetch_add(1, std::memory_order_relaxed) + 1u;
            khps_timed = kh_prof_pick(khps_c, 0u);
        } else {
            khps_timed = true;
        }
        if (khps_timed) khps_t0 = __rdtsc();
    }
    ~KhProfScope() {
        if (!khps_on) return;
        g_prof_count[khps_z].fetch_add(1, std::memory_order_relaxed);
        if (!khps_timed) return;
        const uint64_t khps_t1 = __rdtsc();
        if (khps_t1 <= khps_t0) return;   // KH_PROF_TSC_CLOCK: a backwards read (unsynchronised cores) is dropped.
        const uint64_t khps_dt = khps_t1 - khps_t0;
        g_prof_tsc[khps_z].fetch_add(kh_prof_sampled(khps_z) ? khps_dt * KH_PROF_SAMPLE_EVERY : khps_dt,
                                     std::memory_order_relaxed);
    }
    KhProfScope(const KhProfScope&) = delete;
    KhProfScope& operator=(const KhProfScope&) = delete;
};
#define KH_PROF_SCOPE(z) KhProfScope khps_scope_##z(z)
// An explicit interval for a region a scope cannot bracket (a constructor's
// own body: the graphics lock's acquire). Same accounting, armed only.
inline int64_t kh_prof_now() { if (!kh_stats_on()) return 0; LARGE_INTEGER khpn_q = {}; QueryPerformanceCounter(&khpn_q); return khpn_q.QuadPart; }
inline void kh_prof_add(KhProfZone khpa_z, int64_t khpa_t0) {
    if (!kh_stats_on() || khpa_t0 == 0) return;
    LARGE_INTEGER khpa_q = {}; QueryPerformanceCounter(&khpa_q);
    g_prof_ticks[khpa_z].fetch_add(static_cast<uint64_t>(khpa_q.QuadPart - khpa_t0), std::memory_order_relaxed);
    g_prof_count[khpa_z].fetch_add(1, std::memory_order_relaxed);
}
// KH_PROF_SUB - the steps of one captured upload (KH_PROF_UPLOAD) and of one draw (KH_PROF_DRAWSTEP). Tens of
// thousands of entries a frame, so a QPC pair on each, or an atomic read-modify-write, would itself move the frame:
// one decision per upload / per draw (kh_prof_up_begin / KhProfDsDecision: armed, and one in KH_PROF_SAMPLE_EVERY
// timed), then each step counts with a plain add and, on a timed one, reads the TSC (__rdtsc) twice - scaled by its
// rate (khpb_scale: the sample rate, or 1 for a step timed whole). The fold converts the ticks with the TSC's rate it
// measured against QPC since the last armed fold (an invariant TSC, as QPC itself uses where it has one - without one,
// a CPU older than invariant TSCs, a zone is scaled by the interval's average rate: a measurement error only; a read
// that runs backwards - cores whose TSCs disagree - drops that sample). Written only by the
// render-thread identity (the hooks past reorder_on_render_thread, the funnel behind kh_upload_hook_wanted) and folded
// on it: relaxed loads and stores, no read-modify-write - a race would miscount, not corrupt.
static KhProfZone g_prof_up_cur = KHP_ZONE_N;   // The step in flight (scanExtBytes' owner); KHP_ZONE_N = none.
inline void kh_prof_plain_add(std::atomic<uint64_t>& khpa_a, uint64_t khpa_n) {
    khpa_a.store(khpa_a.load(std::memory_order_relaxed) + khpa_n, std::memory_order_relaxed);
}
// KH_PROF_DRV: the driver's own call inside a sampled hook scope (khpd_s), timed on one call in
// KH_PROF_SAMPLE_EVERY of its own (a second decision from the scope's count) with the TSC, and scaled. Its own
// decision, not the scope's: timed on the scope's timed calls, its cost would sit in every call the scope extrapolates.
// Any thread (the hooks'), so atomics; 0 = not timed (nothing recorded).
inline uint64_t kh_prof_drv_t0(const KhProfScope& khpd_s) {
    if (khpd_s.khps_c == 0u || !kh_prof_pick(khpd_s.khps_c, 1u)) return 0u;
    return __rdtsc();
}
inline void kh_prof_drv(KhProfZone khpd_z, uint64_t khpd_t0) {
    if (khpd_t0 == 0u) return;
    const uint64_t khpd_t1 = __rdtsc();
    if (khpd_t1 <= khpd_t0) return;   // KH_PROF_TSC_CLOCK: a backwards read is dropped.
    g_prof_tsc[khpd_z].fetch_add((khpd_t1 - khpd_t0) * KH_PROF_SAMPLE_EVERY, std::memory_order_relaxed);
    g_prof_count[khpd_z].fetch_add(KH_PROF_SAMPLE_EVERY, std::memory_order_relaxed);
}
// KH_PROF_TSC: a zone timed whole with the TSC, any thread - for a hook called on every draw whose work lands on a
// few of them (a 1-in-16 sample would read 0 or 16x a frame): two TSC reads and two atomic adds per armed call.
struct KhProfTscScope {
    KhProfZone khpt_z;
    uint64_t   khpt_t0;
    bool       khpt_on;
    explicit KhProfTscScope(KhProfZone khpt_zone) : khpt_z(khpt_zone), khpt_t0(0), khpt_on(kh_stats_on()) {
        if (khpt_on) khpt_t0 = __rdtsc();
    }
    ~KhProfTscScope() {
        if (!khpt_on) return;
        g_prof_count[khpt_z].fetch_add(1, std::memory_order_relaxed);
        const uint64_t khpt_t1 = __rdtsc();
        if (khpt_t1 > khpt_t0) g_prof_tsc[khpt_z].fetch_add(khpt_t1 - khpt_t0, std::memory_order_relaxed);
    }
    KhProfTscScope(const KhProfTscScope&) = delete;
    KhProfTscScope& operator=(const KhProfTscScope&) = delete;
};
#define KH_PROF_TSC(z) KhProfTscScope khpt_scope_##z(z)
struct KhProfSub {
    KhProfZone khpb_z;
    KhProfZone khpb_prev;
    uint64_t   khpb_t0;
    uint64_t   khpb_scale;
    bool       khpb_on;
    bool       khpb_timed;
    KhProfSub(KhProfZone khpb_zone, bool khpb_armed, bool khpb_time, uint64_t khpb_rate)
        : khpb_z(khpb_zone), khpb_prev(KHP_ZONE_N), khpb_t0(0), khpb_scale(khpb_rate), khpb_on(khpb_armed),
          khpb_timed(khpb_armed && khpb_time) {
        if (!khpb_on) return;
        kh_prof_plain_add(g_prof_count[khpb_z], 1u);
        khpb_prev = g_prof_up_cur;
        g_prof_up_cur = khpb_z;
        if (khpb_timed) khpb_t0 = __rdtsc();
    }
    ~KhProfSub() {
        if (!khpb_on) return;
        if (khpb_timed) {
            const uint64_t khpb_t1 = __rdtsc();
            if (khpb_t1 > khpb_t0) kh_prof_plain_add(g_prof_tsc[khpb_z], (khpb_t1 - khpb_t0) * khpb_scale);
        }
        g_prof_up_cur = khpb_prev;
    }
    KhProfSub(const KhProfSub&) = delete;
    KhProfSub& operator=(const KhProfSub&) = delete;
};
// A sampled step (timed on the upload's / draw's decision), and a step timed whole whenever it runs armed.
#define KH_PROF_SUB(z, on, timed) KhProfSub khpb_sub_##z((z), (on), (timed), KH_PROF_SAMPLE_EVERY)
#define KH_PROF_SUB_ALL(z, on) KhProfSub khpb_sub_##z((z), (on), true, 1u)
// The decisions (render-thread identity; KH_PROF_SAMPLE_HASH). kh_prof_up_begin: at the head of kh_upload_capture,
// the funnel's first step for every captured upload (Unmap and UpdateSubresource alike); KhProfDsDecision: past
// reorder_pre_draw's render-thread gate, for the rest of the draw. Disarmed: one relaxed load and two plain stores
// each (the bools), and every step (rendering_integration_injection.hpp) reads the bool.
static uint32_t g_prof_up_ctr = 0;
static bool     g_prof_up_on = false;
static bool     g_prof_up_t = false;
inline void kh_prof_up_begin() {
    g_prof_up_on = kh_stats_on();
    g_prof_up_t = g_prof_up_on && kh_prof_pick(++g_prof_up_ctr, 2u);
}
static uint32_t g_prof_ds_ctr = 0;
static bool     g_prof_ds_on = false;
static bool     g_prof_ds_t = false;
// khdd_own: one of our own draws (g_ro.in_injection) - not one of the engine's: no decision of its own, nothing
// counted, and the engine draw it is issued inside (a seam / PIP / in-front injection) gets its decision back.
struct KhProfDsDecision {
    bool khdd_own;
    bool khdd_on;
    bool khdd_t;
    explicit KhProfDsDecision(bool khdd_own_draw) : khdd_own(khdd_own_draw), khdd_on(false), khdd_t(false) {
        const bool khdd_armed = kh_stats_on();
        if (khdd_own && khdd_armed) {
            khdd_on = g_prof_ds_on;
            khdd_t = g_prof_ds_t;
            g_prof_ds_on = false;
            g_prof_ds_t = false;
            return;
        }
        khdd_own = false;
        g_prof_ds_on = khdd_armed;
        g_prof_ds_t = khdd_armed && kh_prof_pick(++g_prof_ds_ctr, 3u);
    }
    ~KhProfDsDecision() {
        if (!khdd_own) return;
        g_prof_ds_on = khdd_on;
        g_prof_ds_t = khdd_t;
    }
    KhProfDsDecision(const KhProfDsDecision&) = delete;
    KhProfDsDecision& operator=(const KhProfDsDecision&) = delete;
};
// KH_PROF_FUNNEL - counts and bytes per cycle (render-thread identity, armed only; plain adds as KhProfSub's). The
// upload funnel: captured uploads (UpdateSubresource's among them), the eager copies and bytes, the write-combined
// fences, the extensions past the eager copy (kh_upload_need / kh_upload_copy_range) and their bytes, the light
// capture's whole copies (kh_cbs_write) and bytes, in all and by what the sampler named the buffer (the want table's
// kind, tagged while armed: cb11, cb10, a VS slot, none yet), uploads the want table refused (a shadow, if any,
// marked stale), captures the idle cadence skipped, and the want table's index slots probed. The light sampler: the
// engine's draws past the render-thread gate, draws with no cb11 bound, uploads already processed this span, shadow
// misses (an incomplete side), twins of an authored window, new lists hashed and the bytes hashed, matches run and
// replayed (KH_DL_PRES_MEMO) and the record x pool-light pairs the runs stood against (records x pool, as before),
// windows authored and the VS bytes they copied; then the pairs the runs visited (KH_DL_PRES_HIX: the lights a
// record's height can match, or the whole pool), the height index's rebuilds and its in-place moves (a presence
// rewrite that moved a light's height). KH_MAP_MIX: the captured maps
// (UpdateSubresource's not among them) by kind - whole-buffer (WRITE_DISCARD) and partial (WRITE_NO_OVERWRITE) -
// each with the buffers' full ByteWidth summed (what copying the buffer whole at Unmap would move), and the
// whole-buffer maps by ByteWidth: <= 256, <= 1 KiB, <= 4 KiB, <= 16 KiB, larger. KH_MAP_REDIRECT: the whole-buffer
// maps handed a cached buffer and their ByteWidth (the bytes the Unmap copies to the driver's pointer), the
// whole-buffer maps with a pending slot that were not (no free record, no cached buffer to give, wider than the
// cap, the hooks not all installed, or KH_MAP_REDIRECT_ON false), and the cached buffers (re)assigned to a buffer (a
// first map, a new width, or the table's churn). KH_MAP_BRANCH: the Map and Unmap hooks' per-call branches - Maps
// through the hook, those whose forget (KH_REPLAY_CBREUSE) took g_rp_cbkeep_mx, those KH_REPLAY_SPLIT looked up in
// its set and those it split, those that scanned the redirect records (kh_redir_drop), those the upload gate
// admitted, and of these the ones given a pending slot and the ones finding every slot held; Unmaps through the
// hook, those that walked the pending slots, those that found theirs, those that scanned the redirect records
// (kh_redir_close); and the forgets that took the mutex from any hook. Counted on the hook scope's armed snapshot
// (khps_on) - the forgets' own count excepted: kh_rp_cb_forget has callers with no such scope and loads
// kh_stats_on() on its locked path, so on the call that straddles an arm or a disarm it and mapReplayLocks can differ
// by one. Behind the render-thread gate, plain adds as above; elsewhere any thread, so atomic adds (kh_fun_any).
enum KhFunnel : uint32_t {
    KHF_UPLOADS, KHF_UPDSUB, KHF_EAGER_N, KHF_EAGER_B, KHF_FENCES, KHF_EXT_N, KHF_EXT_B,
    KHF_SHADOW_N, KHF_SHADOW_B, KHF_SH11_N, KHF_SH11_B, KHF_SH10_N, KHF_SH10_B, KHF_SHVS_N, KHF_SHVS_B,
    KHF_SHOTHER_N, KHF_SHOTHER_B, KHF_WANT_REFUSED, KHF_IDLE_SKIP, KHF_GATE_PROBES,
    KHF_DRAWS, KHF_DL_NOLIST, KHF_DL_SAME, KHF_DL_MISS, KHF_DL_TWIN, KHF_DL_LISTS, KHF_DL_HASH_B,
    KHF_DL_RUNS, KHF_DL_REPLAYS, KHF_DL_PAIRS, KHF_DL_WINDOWS, KHF_DL_WIN_VS_B,
    KHF_DL_VISITS, KHF_DL_HIX_BUILDS, KHF_DL_HIX_MOVES,   // KH_DL_PRES_HIX.
    KHF_MAP_DISC_N, KHF_MAP_DISC_B, KHF_MAP_NOOV_N, KHF_MAP_NOOV_B,   // KH_MAP_MIX.
    KHF_MAP_W256, KHF_MAP_W1K, KHF_MAP_W4K, KHF_MAP_W16K, KHF_MAP_WBIG,
    KHF_REDIR_N, KHF_REDIR_B, KHF_REDIR_REFUSED, KHF_REDIR_BUFS,   // KH_MAP_REDIRECT.
    KHF_HK_MAP_N, KHF_HK_RP_LOCK, KHF_HK_RP_CHECK, KHF_HK_RP_SPLIT, KHF_HK_RD_DROP,   // KH_MAP_BRANCH.
    KHF_HK_ADMIT, KHF_HK_PENDING, KHF_HK_NOSLOT,
    KHF_HK_UNMAP_N, KHF_HK_UN_WALK, KHF_HK_UN_HIT, KHF_HK_RD_CLOSE, KHF_RP_LOCKS,
    KHF_N
};
static const char* const g_fun_name[] = {
    "uploads", "updSubUploads", "eagerCopies", "eagerBytes", "wcFences", "extCopies", "extBytes",
    "shadowCopies", "shadowBytes", "shadowCb11Copies", "shadowCb11Bytes", "shadowCb10Copies", "shadowCb10Bytes",
    "shadowVsCopies", "shadowVsBytes", "shadowOtherCopies", "shadowOtherBytes", "wantRefused", "idleSkips",
    "gateProbes",
    "draws", "dlNoList", "dlSameUpload", "dlShadowMiss", "dlTwin", "dlNewLists", "dlHashBytes",
    "dlMatchRuns", "dlMatchReplays", "dlMatchPairs", "dlWindows", "dlWindowVsBytes",
    "dlMatchVisited", "dlHeightIxBuilds", "dlHeightIxMoves",
    "mapDiscard", "mapDiscardBytes", "mapNoOverwrite", "mapNoOverwriteBytes",
    "mapW256", "mapW1k", "mapW4k", "mapW16k", "mapWBig",
    "mapRedirected", "mapRedirectedBytes", "mapRedirectRefused", "mapRedirectBuffers",
    "mapCalls", "mapReplayLocks", "mapReplayChecks", "mapReplaySplits", "mapRedirectDrops",
    "mapAdmitted", "mapPending", "mapNoSlot",
    "unmapCalls", "unmapSlotWalks", "unmapSlotHits", "unmapRedirectCloses", "replayLocks",
};
static_assert(sizeof(g_fun_name) / sizeof(g_fun_name[0]) == KHF_N, "KH_PROF_FUNNEL: a name per counter");
static std::atomic<uint64_t> g_fun_acc[KHF_N];   // This cycle's (render-thread identity).
static std::atomic<uint64_t> g_fun_pub[KHF_N];   // The ended cycle's (the fold).
static std::atomic<uint64_t> g_fun_win[KHF_N];   // Summed over the window (KH_PROF_WINDOW).
inline void kh_fun(KhFunnel khfn_k, uint64_t khfn_n) { kh_prof_plain_add(g_fun_acc[khfn_k], khfn_n); }
// KH_MAP_BRANCH: a count made off the render-thread gate (any thread): an atomic add, armed callers only, and only to
// a counter kh_fun never adds to.
inline void kh_fun_any(KhFunnel khfa_k, uint64_t khfa_n) {
    g_fun_acc[khfa_k].fetch_add(khfa_n, std::memory_order_relaxed);
}
// The want table's kinds (kh_dl_want_note's tag; a buffer named in more than one role carries each bit).
static constexpr uint8_t KH_DL_KIND_CB11 = 1u;
static constexpr uint8_t KH_DL_KIND_CB10 = 2u;
static constexpr uint8_t KH_DL_KIND_VS = 4u;
inline void kh_fun_shadow(uint8_t khfs_kind, uint64_t khfs_b) {
    kh_fun(KHF_SHADOW_N, 1u);
    kh_fun(KHF_SHADOW_B, khfs_b);
    if (khfs_kind & KH_DL_KIND_CB11)     { kh_fun(KHF_SH11_N, 1u); kh_fun(KHF_SH11_B, khfs_b); }
    else if (khfs_kind & KH_DL_KIND_CB10) { kh_fun(KHF_SH10_N, 1u); kh_fun(KHF_SH10_B, khfs_b); }
    else if (khfs_kind & KH_DL_KIND_VS)   { kh_fun(KHF_SHVS_N, 1u); kh_fun(KHF_SHVS_B, khfs_b); }
    else                                  { kh_fun(KHF_SHOTHER_N, 1u); kh_fun(KHF_SHOTHER_B, khfs_b); }
}
// KH_MAP_MIX: one captured map - its kind and its buffer's full ByteWidth (armed callers only).
inline void kh_fun_map_mix(bool khmm_discard, uint32_t khmm_w) {
    if (!khmm_discard) {
        kh_fun(KHF_MAP_NOOV_N, 1u);
        kh_fun(KHF_MAP_NOOV_B, khmm_w);
        return;
    }
    kh_fun(KHF_MAP_DISC_N, 1u);
    kh_fun(KHF_MAP_DISC_B, khmm_w);
    if (khmm_w <= 256u)        kh_fun(KHF_MAP_W256, 1u);
    else if (khmm_w <= 1024u)  kh_fun(KHF_MAP_W1K, 1u);
    else if (khmm_w <= 4096u)  kh_fun(KHF_MAP_W4K, 1u);
    else if (khmm_w <= 16384u) kh_fun(KHF_MAP_W16K, 1u);
    else                       kh_fun(KHF_MAP_WBIG, 1u);
}
// scanExtBytes: the extension bytes each step made the funnel copy (the step in flight, g_prof_up_cur).
static std::atomic<uint64_t> g_prof_xb_acc[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_xb_pub[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_xb_win[KHP_ZONE_N];
inline void kh_fun_ext(uint64_t khfe_b) {
    kh_fun(KHF_EXT_N, 1u);
    kh_fun(KHF_EXT_B, khfe_b);
    if (g_prof_up_cur < KHP_ZONE_N) kh_prof_plain_add(g_prof_xb_acc[g_prof_up_cur], khfe_b);
}
// KH_PROF_WINDOW - one cycle is one sample: a 1-in-16 zone multiplies one interrupted call by 16, and a step that runs
// once a frame lands on the cycle or not. Every whole armed cycle (a published fold, KH_PROF_DISARM, whose previous
// fold was armed too) also adds to these, so getRenderStats can report each zone's mean and worst cycle since the
// arming or the last resetRenderStats.
static std::atomic<uint64_t> g_prof_win_cycles{ 0 };
static std::atomic<uint64_t> g_prof_win_us[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_win_n[KHP_ZONE_N];
static std::atomic<uint64_t> g_prof_win_max[KHP_ZONE_N];
// KH_PROF_SUB's rate: TSC ticks per microsecond, measured at each armed fold over the interval since the previous one
// - the conversion of every TSC-timed zone (every CPU zone but kh_prof_add's QPC intervals). 0 until two armed folds
// have run in this process (those zones' time then publishes as 0); kept across resets - the machine's rate,
// measured again by every armed fold. Render-thread identity writes.
static uint64_t g_prof_cal_q = 0;
static uint64_t g_prof_cal_t = 0;
static std::atomic<double> g_prof_tsc_per_us{ 0.0 };
// KH_PROF_WINDOW: the last fold found collection on (render thread). The window takes whole armed cycles only: the
// cycle the arming split stays out of the means (its fold publishes the counts and kh_prof_add's QPC zones - nothing
// at all when it is also the first fold after a reset, KH_PROF_DISARM; the TSC zones, scanExtBytes and the funnel
// read 0 there).
static bool g_prof_prev_on = false;

enum KhGpuZone : uint32_t {
    KHG_INJECT, KHG_SUN_LADDER, KHG_DLS_FRAME, KHG_DLS_WORLD, KHG_MASK_CAST, KHG_SVS_PRIME,
    KHG_SVS_VOL_COPY, KHG_VMIR, KHG_SSAO_POST, KHG_SNAPSHOT, KHG_PIP,
    KHG_FLUSH, KHG_FLUSH_UI, KHG_SCENE_CAPTURE, KHG_DLF,
    KHG_ZONE_N
};
static const char* const g_gpu_zone_name[KHG_ZONE_N] = {
    "gInject", "gSunLadder", "gDlsFrame", "gDlsWorld", "gMaskCast", "gSvsPrime",
    "gSvsVolCopy", "gVmir", "gSsaoPost", "gSnapshot", "gPip",
    "gFlush", "gFlushUi", "gSceneCapture", "gDlFog",
};
static constexpr uint32_t KH_GPU_RING = 8u;
// The cycles back a slot is read, never waiting (< KH_GPU_RING): the GPU was measured running 5 cycles behind the CPU
// under load at 4K, and a slot read sooner is never done. The ring reopens it KH_GPU_RING - KH_GPU_LAG folds later.
static constexpr uint32_t KH_GPU_LAG = 6u;
// KH_GPU_PASS: the effect passes a cycle times (the scene chain's and the UI chain's), each its own timestamp pair in
// the cycle's bracket, keyed by kh_gpu_pass_key; past this many in a cycle the rest go untimed.
static constexpr uint32_t KH_GPU_PASS_N = 32u;
struct KhGpuProf {
    ID3D11Query* disjoint[KH_GPU_RING] = {};
    ID3D11Query* ts[KHG_ZONE_N][KH_GPU_RING][2] = {};
    bool         used[KHG_ZONE_N][KH_GPU_RING] = {};
    ID3D11Query* pts[KH_GPU_RING][KH_GPU_PASS_N][2] = {};   // KH_GPU_PASS.
    uint32_t     pass_n[KH_GPU_RING] = {};
    uint32_t     pass_key[KH_GPU_RING][KH_GPU_PASS_N] = {};
    bool         pass_used[KH_GPU_RING][KH_GPU_PASS_N] = {};
    bool         ready = false;    // Queries exist.
    bool         failed = false;   // A create failed: the GPU side stands down for the session.
    int          open_slot = -1;   // Slot whose disjoint is begun, -1 = none.
    uint64_t     cycle = 0;        // Cycles folded (ring index source).
};
static KhGpuProf g_gpu_prof;
static std::atomic<uint64_t> g_gpu_us_pub[KHG_ZONE_N];
// KH_PROF_WINDOW: every GPU value the fold harvested since the arming / reset (sum, count, worst).
static std::atomic<uint64_t> g_gpu_win_us[KHG_ZONE_N];
static std::atomic<uint64_t> g_gpu_win_n[KHG_ZONE_N];
static std::atomic<uint64_t> g_gpu_win_max[KHG_ZONE_N];
// KH_GPU_PASS: a pass's key - the effect id (0 - 255), the chain (bit 8: 0 the scene's, 1 the UI's), a user shader
// (bit 9), the stages fused into it (bits 10 - 15, at most 63).
inline uint32_t kh_gpu_pass_key(uint32_t khgk_chain, int khgk_effect, bool khgk_user, uint32_t khgk_fused) {
    const uint32_t khgk_e = khgk_effect < 0 ? 0u : (khgk_effect > 255 ? 255u : static_cast<uint32_t>(khgk_effect));
    const uint32_t khgk_f = khgk_fused > 63u ? 63u : khgk_fused;
    return khgk_e | ((khgk_chain & 1u) << 8) | ((khgk_user ? 1u : 0u) << 9) | (khgk_f << 10);
}
// The harvested cycle's passes (key << 32 | microseconds), in draw order, and how many; and the window per (chain,
// user, effect) - the key's low KH_GPU_PASS_W bits: sum, count, worst.
static constexpr uint32_t KH_GPU_PASS_W = 1024u;
static std::atomic<uint64_t> g_gpu_pass_pub[KH_GPU_PASS_N];
static std::atomic<uint32_t> g_gpu_pass_pub_n{ 0 };
static std::atomic<uint64_t> g_gpu_passw_us[KH_GPU_PASS_W];
static std::atomic<uint64_t> g_gpu_passw_n[KH_GPU_PASS_W];
static std::atomic<uint64_t> g_gpu_passw_max[KH_GPU_PASS_W];
// KH_PROF_WINDOW / KH_PROF_FUNNEL / KH_GPU_PASS: the window starts again and the published values go to zero - by
// resetRenderStats (kh_prof_reset_pub) and by the first fold after it (KH_PROF_DISARM).
inline void kh_prof_window_zero() {
    g_prof_win_cycles.store(0, std::memory_order_relaxed);
    for (uint32_t khwz_z = 0; khwz_z < KHP_ZONE_N; ++khwz_z) {
        g_prof_win_us[khwz_z].store(0, std::memory_order_relaxed);
        g_prof_win_n[khwz_z].store(0, std::memory_order_relaxed);
        g_prof_win_max[khwz_z].store(0, std::memory_order_relaxed);
        g_prof_xb_pub[khwz_z].store(0, std::memory_order_relaxed);
        g_prof_xb_win[khwz_z].store(0, std::memory_order_relaxed);
    }
    for (uint32_t khwz_z = 0; khwz_z < KHG_ZONE_N; ++khwz_z) {
        g_gpu_win_us[khwz_z].store(0, std::memory_order_relaxed);
        g_gpu_win_n[khwz_z].store(0, std::memory_order_relaxed);
        g_gpu_win_max[khwz_z].store(0, std::memory_order_relaxed);
    }
    for (uint32_t khwz_k = 0; khwz_k < KHF_N; ++khwz_k) {
        g_fun_pub[khwz_k].store(0, std::memory_order_relaxed);
        g_fun_win[khwz_k].store(0, std::memory_order_relaxed);
    }
    g_gpu_pass_pub_n.store(0, std::memory_order_relaxed);
    for (uint32_t khwz_w = 0; khwz_w < KH_GPU_PASS_W; ++khwz_w) {
        g_gpu_passw_us[khwz_w].store(0, std::memory_order_relaxed);
        g_gpu_passw_n[khwz_w].store(0, std::memory_order_relaxed);
        g_gpu_passw_max[khwz_w].store(0, std::memory_order_relaxed);
    }
}
inline uint32_t kh_gpu_slot() { return static_cast<uint32_t>(g_gpu_prof.cycle % KH_GPU_RING); }
inline bool kh_gpu_prof_ensure(ID3D11DeviceContext* khgp_ctx) {
    if (g_gpu_prof.ready) return true;
    if (g_gpu_prof.failed || !khgp_ctx) return false;
    ID3D11Device* khgp_dev = nullptr;
    khgp_ctx->GetDevice(&khgp_dev);
    if (!khgp_dev) { g_gpu_prof.failed = true; return false; }
    bool khgp_ok = true;
    for (uint32_t khgp_r = 0; khgp_r < KH_GPU_RING && khgp_ok; ++khgp_r) {
        D3D11_QUERY_DESC khgp_dd = {}; khgp_dd.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
        if (FAILED(khgp_dev->CreateQuery(&khgp_dd, &g_gpu_prof.disjoint[khgp_r]))) khgp_ok = false;
        for (uint32_t khgp_z = 0; khgp_z < KHG_ZONE_N && khgp_ok; ++khgp_z) {
            D3D11_QUERY_DESC khgp_td = {}; khgp_td.Query = D3D11_QUERY_TIMESTAMP;
            if (FAILED(khgp_dev->CreateQuery(&khgp_td, &g_gpu_prof.ts[khgp_z][khgp_r][0])) ||
                FAILED(khgp_dev->CreateQuery(&khgp_td, &g_gpu_prof.ts[khgp_z][khgp_r][1]))) khgp_ok = false;
        }
        for (uint32_t khgp_p = 0; khgp_p < KH_GPU_PASS_N && khgp_ok; ++khgp_p) {   // KH_GPU_PASS.
            D3D11_QUERY_DESC khgp_pd = {}; khgp_pd.Query = D3D11_QUERY_TIMESTAMP;
            if (FAILED(khgp_dev->CreateQuery(&khgp_pd, &g_gpu_prof.pts[khgp_r][khgp_p][0])) ||
                FAILED(khgp_dev->CreateQuery(&khgp_pd, &g_gpu_prof.pts[khgp_r][khgp_p][1]))) khgp_ok = false;
        }
    }
    khgp_dev->Release();
    if (!khgp_ok) {
        for (uint32_t khgp_r = 0; khgp_r < KH_GPU_RING; ++khgp_r) {
            KH_SAFE_RELEASE(g_gpu_prof.disjoint[khgp_r]);
            for (uint32_t khgp_z = 0; khgp_z < KHG_ZONE_N; ++khgp_z) { KH_SAFE_RELEASE(g_gpu_prof.ts[khgp_z][khgp_r][0]); KH_SAFE_RELEASE(g_gpu_prof.ts[khgp_z][khgp_r][1]); }
            for (uint32_t khgp_p = 0; khgp_p < KH_GPU_PASS_N; ++khgp_p) {   // KH_GPU_PASS.
                KH_SAFE_RELEASE(g_gpu_prof.pts[khgp_r][khgp_p][0]);
                KH_SAFE_RELEASE(g_gpu_prof.pts[khgp_r][khgp_p][1]);
            }
        }
        g_gpu_prof.failed = true;
        return false;
    }
    g_gpu_prof.ready = true;
    return true;
}
inline void kh_gpu_prof_release() {
    for (uint32_t khgr_r = 0; khgr_r < KH_GPU_RING; ++khgr_r) {
        KH_SAFE_RELEASE(g_gpu_prof.disjoint[khgr_r]);
        for (uint32_t khgr_z = 0; khgr_z < KHG_ZONE_N; ++khgr_z) {
            KH_SAFE_RELEASE(g_gpu_prof.ts[khgr_z][khgr_r][0]);
            KH_SAFE_RELEASE(g_gpu_prof.ts[khgr_z][khgr_r][1]);
            g_gpu_prof.used[khgr_z][khgr_r] = false;
        }
        for (uint32_t khgr_p = 0; khgr_p < KH_GPU_PASS_N; ++khgr_p) {   // KH_GPU_PASS.
            KH_SAFE_RELEASE(g_gpu_prof.pts[khgr_r][khgr_p][0]);
            KH_SAFE_RELEASE(g_gpu_prof.pts[khgr_r][khgr_p][1]);
            g_gpu_prof.pass_used[khgr_r][khgr_p] = false;
        }
        g_gpu_prof.pass_n[khgr_r] = 0;
    }
    g_gpu_prof.ready = false;
    g_gpu_prof.failed = false;
    g_gpu_prof.open_slot = -1;
}
struct KhGpuScope {
    ID3D11DeviceContext* khgs_ctx;
    KhGpuZone            khgs_z;
    int                  khgs_slot;   // The bracket the begin stamp went to; -1 = none.
    KhGpuScope(ID3D11DeviceContext* khgs_c, KhGpuZone khgs_zone) : khgs_ctx(khgs_c), khgs_z(khgs_zone), khgs_slot(-1) {
        // Only inside an open cycle bracket, on a ready query set.
        if (!kh_stats_on() || !khgs_ctx || g_gpu_prof.open_slot < 0 || !g_gpu_prof.ready) return;
        khgs_slot = g_gpu_prof.open_slot;
        khgs_ctx->End(g_gpu_prof.ts[khgs_z][static_cast<uint32_t>(khgs_slot)][0]);
        g_gpu_prof.used[khgs_z][static_cast<uint32_t>(khgs_slot)] = true;
    }
    ~KhGpuScope() {
        // The end stamp goes to the begin's bracket; if that bracket closed
        // meanwhile the pair is dropped rather than straddling two.
        if (khgs_slot < 0) return;
        if (g_gpu_prof.open_slot != khgs_slot) { g_gpu_prof.used[khgs_z][static_cast<uint32_t>(khgs_slot)] = false; return; }
        khgs_ctx->End(g_gpu_prof.ts[khgs_z][static_cast<uint32_t>(khgs_slot)][1]);
    }
    KhGpuScope(const KhGpuScope&) = delete;
    KhGpuScope& operator=(const KhGpuScope&) = delete;
};
#define KH_GPU_SCOPE(ctx, z) KhGpuScope khgs_scope_##z((ctx), z)
// KH_GPU_PASS: one effect pass's GPU interval - KhGpuScope's rules (only inside an open cycle bracket, on a ready
// query set; an end that finds its bracket closed drops the pair), one of the cycle's KH_GPU_PASS_N pairs.
struct KhGpuPassScope {
    ID3D11DeviceContext* khgq_ctx;
    int                  khgq_slot;   // The bracket the begin stamp went to; -1 = none.
    uint32_t             khgq_i;
    KhGpuPassScope(ID3D11DeviceContext* khgq_c, uint32_t khgq_chain, int khgq_effect, bool khgq_user,
                   uint32_t khgq_fused) : khgq_ctx(khgq_c), khgq_slot(-1), khgq_i(0) {
        if (!kh_stats_on() || !khgq_ctx || g_gpu_prof.open_slot < 0 || !g_gpu_prof.ready) return;
        const uint32_t khgq_s = static_cast<uint32_t>(g_gpu_prof.open_slot);
        if (g_gpu_prof.pass_n[khgq_s] >= KH_GPU_PASS_N) return;
        khgq_i = g_gpu_prof.pass_n[khgq_s]++;
        g_gpu_prof.pass_key[khgq_s][khgq_i] = kh_gpu_pass_key(khgq_chain, khgq_effect, khgq_user, khgq_fused);
        g_gpu_prof.pass_used[khgq_s][khgq_i] = true;
        khgq_ctx->End(g_gpu_prof.pts[khgq_s][khgq_i][0]);
        khgq_slot = static_cast<int>(khgq_s);
    }
    ~KhGpuPassScope() {
        if (khgq_slot < 0) return;
        if (g_gpu_prof.open_slot != khgq_slot) {
            g_gpu_prof.pass_used[static_cast<uint32_t>(khgq_slot)][khgq_i] = false;
            return;
        }
        khgq_ctx->End(g_gpu_prof.pts[static_cast<uint32_t>(khgq_slot)][khgq_i][1]);
    }
    KhGpuPassScope(const KhGpuPassScope&) = delete;
    KhGpuPassScope& operator=(const KhGpuPassScope&) = delete;
};

// The cycle fold, at the main depth clear on the render thread: publish the
// CPU accumulators, close this cycle's GPU bracket, harvest the slot
// KH_GPU_LAG cycles back, and open the next bracket while the stats stay armed.
inline void kh_prof_fold(ID3D11DeviceContext* khpf_ctx) {
    const uint64_t khpf_f = kh_prof_freq();
    const bool khpf_on = kh_stats_on();
    // KH_PROF_DISARM: publish only what was measured armed and since the last reset.
    const uint32_t khpf_ep = g_prof_epoch.load(std::memory_order_acquire);
    const bool khpf_pub = khpf_on && khpf_ep == g_prof_epoch_seen;
    const bool khpf_reset = khpf_ep != g_prof_epoch_seen;   // A resetRenderStats since the last fold.
    g_prof_epoch_seen = khpf_ep;
    // KH_PROF_WINDOW: a whole armed cycle. The TSC zones, scanExtBytes and the funnel publish these only: a scope
    // still open at a disarm adds after it, and nothing drains them disarmed - the arming fold drops what they hold.
    const bool khpf_win = khpf_pub && g_prof_prev_on;
    g_prof_prev_on = khpf_on;
    // KH_PROF_DISARM for the window, the funnel and the passes: the first fold after a reset, armed or not, drops the
    // armed-only accumulators and zeroes the window and what is published, on this thread (a fold that raced a reset
    // made without the graphics lock may have added pre-reset values after the reset zeroed them). Once a reset.
    if (khpf_reset) {
        for (uint32_t khpf_z = 0; khpf_z < KHP_ZONE_N; ++khpf_z) {
            g_prof_tsc[khpf_z].store(0, std::memory_order_relaxed);
            g_prof_xb_acc[khpf_z].store(0, std::memory_order_relaxed);
        }
        for (uint32_t khpf_k = 0; khpf_k < KHF_N; ++khpf_k) g_fun_acc[khpf_k].store(0, std::memory_order_relaxed);
        kh_prof_window_zero();
    }
    // KH_PROF_SUB: the TSC's rate over the interval since the last armed fold. Disarmed folds skip the new work below
    // (nothing armed-only accumulates while disarmed: disarming is a reset, whose leftovers the block above dropped).
    double khpf_tpu = 0.0;
    if (khpf_on) {
        LARGE_INTEGER khpf_q = {};
        QueryPerformanceCounter(&khpf_q);
        const uint64_t khpf_qn = static_cast<uint64_t>(khpf_q.QuadPart);
        const uint64_t khpf_tn = __rdtsc();
        if (g_prof_cal_q != 0 && khpf_qn > g_prof_cal_q && khpf_tn > g_prof_cal_t) {
            g_prof_tsc_per_us.store(static_cast<double>(khpf_tn - g_prof_cal_t) * static_cast<double>(khpf_f) /
                                    (static_cast<double>(khpf_qn - g_prof_cal_q) * 1.0e6), std::memory_order_relaxed);
        }
        g_prof_cal_q = khpf_qn;
        g_prof_cal_t = khpf_tn;
        khpf_tpu = g_prof_tsc_per_us.load(std::memory_order_relaxed);
        if (khpf_win) g_prof_win_cycles.fetch_add(1u, std::memory_order_relaxed);   // KH_PROF_WINDOW.
    }
    for (uint32_t khpf_z = 0; khpf_z < KHP_ZONE_N; ++khpf_z) {
        const uint64_t khpf_t = g_prof_ticks[khpf_z].exchange(0, std::memory_order_relaxed);
        const uint64_t khpf_n = g_prof_count[khpf_z].exchange(0, std::memory_order_relaxed);
        uint64_t khpf_us = khpf_pub ? khpf_t * 1000000ull / khpf_f : 0ull;
        if (khpf_on) {   // KH_PROF_SUB / KH_PROF_UPLOAD / KH_PROF_WINDOW.
            const uint64_t khpf_s = g_prof_tsc[khpf_z].exchange(0, std::memory_order_relaxed);
            const uint64_t khpf_x = g_prof_xb_acc[khpf_z].exchange(0, std::memory_order_relaxed);
            if (khpf_win && khpf_s != 0 && khpf_tpu > 0.0) {
                khpf_us += static_cast<uint64_t>(static_cast<double>(khpf_s) / khpf_tpu);
            }
            g_prof_xb_pub[khpf_z].store(khpf_win ? khpf_x : 0ull, std::memory_order_relaxed);
            if (khpf_win) {
                g_prof_win_us[khpf_z].fetch_add(khpf_us, std::memory_order_relaxed);
                g_prof_win_n[khpf_z].fetch_add(khpf_n, std::memory_order_relaxed);
                g_prof_xb_win[khpf_z].fetch_add(khpf_x, std::memory_order_relaxed);
                if (khpf_us > g_prof_win_max[khpf_z].load(std::memory_order_relaxed)) {
                    g_prof_win_max[khpf_z].store(khpf_us, std::memory_order_relaxed);
                }
            }
        }
        g_prof_us_pub[khpf_z].store(khpf_pub ? khpf_us : 0ull, std::memory_order_relaxed);
        g_prof_n_pub[khpf_z].store(khpf_pub ? khpf_n : 0ull, std::memory_order_relaxed);
    }
    if (khpf_on) {   // KH_PROF_FUNNEL.
        for (uint32_t khpf_k = 0; khpf_k < KHF_N; ++khpf_k) {
            const uint64_t khpf_v = g_fun_acc[khpf_k].exchange(0, std::memory_order_relaxed);
            g_fun_pub[khpf_k].store(khpf_win ? khpf_v : 0ull, std::memory_order_relaxed);
            if (khpf_win) g_fun_win[khpf_k].fetch_add(khpf_v, std::memory_order_relaxed);
        }
    }
    if (!khpf_pub) {   // KH_PROF_DISARM: the GPU times too (the note above g_prof_epoch).
        for (uint32_t khpf_z = 0; khpf_z < KHG_ZONE_N; ++khpf_z) {
            g_gpu_us_pub[khpf_z].store(0, std::memory_order_relaxed);
        }
    }
    if (g_gpu_prof.open_slot >= 0 && g_gpu_prof.ready && khpf_ctx) {
        khpf_ctx->End(g_gpu_prof.disjoint[static_cast<uint32_t>(g_gpu_prof.open_slot)]);
        g_gpu_prof.open_slot = -1;
        g_gpu_prof.cycle++;
        if (!khpf_pub) {   // KH_PROF_DISARM: no slot's stamps outlive it to be read after the next arming.
            for (uint32_t khpf_z = 0; khpf_z < KHG_ZONE_N; ++khpf_z) {
                for (uint32_t khpf_r = 0; khpf_r < KH_GPU_RING; ++khpf_r) g_gpu_prof.used[khpf_z][khpf_r] = false;
            }
            for (uint32_t khpf_r = 0; khpf_r < KH_GPU_RING; ++khpf_r) g_gpu_prof.pass_n[khpf_r] = 0;   // KH_GPU_PASS.
        }
        // Harvest: the slot KH_GPU_LAG cycles back, never waiting.
        if (khpf_pub && g_gpu_prof.cycle >= KH_GPU_LAG) {
            const uint32_t khpf_s = static_cast<uint32_t>((g_gpu_prof.cycle - KH_GPU_LAG) % KH_GPU_RING);
            D3D11_QUERY_DATA_TIMESTAMP_DISJOINT khpf_dj = {};
            if (khpf_ctx->GetData(g_gpu_prof.disjoint[khpf_s], &khpf_dj, sizeof(khpf_dj), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                !khpf_dj.Disjoint && khpf_dj.Frequency > 0) {
                for (uint32_t khpf_z = 0; khpf_z < KHG_ZONE_N; ++khpf_z) {
                    if (!g_gpu_prof.used[khpf_z][khpf_s]) { g_gpu_us_pub[khpf_z].store(0, std::memory_order_relaxed); continue; }
                    UINT64 khpf_a = 0, khpf_b = 0;
                    if (khpf_ctx->GetData(g_gpu_prof.ts[khpf_z][khpf_s][0], &khpf_a, sizeof(khpf_a), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                        khpf_ctx->GetData(g_gpu_prof.ts[khpf_z][khpf_s][1], &khpf_b, sizeof(khpf_b), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_OK &&
                        khpf_b >= khpf_a) {
                        const uint64_t khpf_g = (khpf_b - khpf_a) * 1000000ull / khpf_dj.Frequency;
                        g_gpu_us_pub[khpf_z].store(khpf_g, std::memory_order_relaxed);
                        g_gpu_win_us[khpf_z].fetch_add(khpf_g, std::memory_order_relaxed);   // KH_PROF_WINDOW.
                        g_gpu_win_n[khpf_z].fetch_add(1u, std::memory_order_relaxed);
                        if (khpf_g > g_gpu_win_max[khpf_z].load(std::memory_order_relaxed)) {
                            g_gpu_win_max[khpf_z].store(khpf_g, std::memory_order_relaxed);
                        }
                    }
                    g_gpu_prof.used[khpf_z][khpf_s] = false;
                }
                // KH_GPU_PASS: the slot's passes, in draw order; the published list is this cycle's.
                uint32_t khpf_pk = 0;
                for (uint32_t khpf_i = 0; khpf_i < g_gpu_prof.pass_n[khpf_s]; ++khpf_i) {
                    if (!g_gpu_prof.pass_used[khpf_s][khpf_i]) continue;
                    g_gpu_prof.pass_used[khpf_s][khpf_i] = false;
                    UINT64 khpf_a = 0, khpf_b = 0;
                    ID3D11Query* const* const khpf_q = g_gpu_prof.pts[khpf_s][khpf_i];
                    const UINT khpf_fl = D3D11_ASYNC_GETDATA_DONOTFLUSH;
                    if (khpf_ctx->GetData(khpf_q[0], &khpf_a, sizeof(khpf_a), khpf_fl) != S_OK ||
                        khpf_ctx->GetData(khpf_q[1], &khpf_b, sizeof(khpf_b), khpf_fl) != S_OK ||
                        khpf_b < khpf_a) continue;
                    const uint64_t khpf_g = (khpf_b - khpf_a) * 1000000ull / khpf_dj.Frequency;
                    const uint32_t khpf_key = g_gpu_prof.pass_key[khpf_s][khpf_i];
                    g_gpu_pass_pub[khpf_pk++].store((static_cast<uint64_t>(khpf_key) << 32) | (khpf_g & 0xFFFFFFFFull),
                                                    std::memory_order_relaxed);
                    const uint32_t khpf_w = khpf_key & (KH_GPU_PASS_W - 1u);
                    g_gpu_passw_us[khpf_w].fetch_add(khpf_g, std::memory_order_relaxed);
                    g_gpu_passw_n[khpf_w].fetch_add(1u, std::memory_order_relaxed);
                    if (khpf_g > g_gpu_passw_max[khpf_w].load(std::memory_order_relaxed)) {
                        g_gpu_passw_max[khpf_w].store(khpf_g, std::memory_order_relaxed);
                    }
                }
                g_gpu_pass_pub_n.store(khpf_pk, std::memory_order_relaxed);
            }
        }
    }
    if (khpf_on && khpf_ctx && kh_gpu_prof_ensure(khpf_ctx)) {
        const uint32_t khpf_s = kh_gpu_slot();
        for (uint32_t khpf_z = 0; khpf_z < KHG_ZONE_N; ++khpf_z) g_gpu_prof.used[khpf_z][khpf_s] = false;
        for (uint32_t khpf_i = 0; khpf_i < KH_GPU_PASS_N; ++khpf_i) g_gpu_prof.pass_used[khpf_s][khpf_i] = false;
        g_gpu_prof.pass_n[khpf_s] = 0;   // KH_GPU_PASS.
        khpf_ctx->Begin(g_gpu_prof.disjoint[khpf_s]);
        g_gpu_prof.open_slot = static_cast<int>(khpf_s);
    }
}
inline void kh_prof_reset_pub() {
    for (uint32_t khpr_z = 0; khpr_z < KHP_ZONE_N; ++khpr_z) { g_prof_us_pub[khpr_z].store(0, std::memory_order_relaxed); g_prof_n_pub[khpr_z].store(0, std::memory_order_relaxed); }
    for (uint32_t khpr_z = 0; khpr_z < KHG_ZONE_N; ++khpr_z) g_gpu_us_pub[khpr_z].store(0, std::memory_order_relaxed);
    kh_prof_window_zero();   // KH_PROF_WINDOW (the cycle in flight's accumulators are the fold's to drop).
}

enum class DepthMode : int {
    TestOnly = 0,   // Default: occluded by scene, depth buffer untouched.
    TestWrite = 1,
    Off = 2,
};

struct MeshVertex {
    float pos[3];   // Offset 0 (contract: never moves - both input layouts).
    float nrm[3];   // Offset 12 (contract: never moves).
    float uv[2];   // Offset 24 - texture lane (builtins carry usable UVs).
    float tan[4];   // Offset 32 - tangent xyz, handedness w (mikktspace); builtins analytic (meshgen), w = +-1.
};

// Per-material index range into MeshDef::indices. Builtins carry one full-range
// "default" entry so no consumer special-cases them. Every draw is
// DrawIndexed(index_count, index_start, 0): indices are absolute, base vertex
// 0.
struct MeshSubmesh {
    std::string name;
    uint32_t index_start = 0;
    uint32_t index_count = 0;
};

// Submesh count is invariant across LOD levels (every level has one range per
// submesh, in order: KH_LOD_JOINT decimates them together and a triangle keeps
// its submesh; a small submesh may draw nothing at a coarse level), so a
// material slot index addresses the same surface at every level.
static constexpr int KH_LOD_MAX = 12;   // 13 total with level 0.

// KH_SKEL: one bone of an imported skeleton, and one vertex's influences. Both
// are written raw into the mesh cache (KH_SKIN_CACHE_MAGIC), so KhSkinInf keeps
// a fixed layout.
struct KhSkinBone {
    std::string name;       // Lower-cased: matched to memory point names case-insensitively.
    int32_t parent = -1;    // Index into MeshDef::skin_bones; -1 = a root of the skeleton.
    float head[3] = { 0.0f, 0.0f, 0.0f };   // Bind position: engine axes, metres, authored frame.
};
struct KhSkinInf {
    uint16_t b[4] = { 0, 0, 0, 0 };           // MeshDef::skin_bones indices.
    // Summing to 1 or under: what is under 1 follows the root (all 0: the whole vertex does). An import's sum
    // is 1 or 0; a decimated vertex between weighted and unweighted ones carries a part (KH_LOD_SKIN).
    float w[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};
static_assert(sizeof(KhSkinInf) == 24, "KhSkinInf is cached raw: four uint16 then four float");

struct MeshDef {
    std::string name;   // Primary name (SQF mesh selector, lower-case).
    std::string alias;   // Accepted alternative ("" = none).
    // Welded vertices + a triangle-list index stream laid out [level 0][level
    // 1]...[level lod_n]. Builders produce an expanded triangle list in verts
    // with ranges in triangle-vertex units; kh_mesh_weld dedups and fills
    // indices, then reorders the triangles inside each range for the vertex
    // cache (kh_mesh_cache_order), so the ranges become index ranges with their
    // starts and counts unchanged.
    // indices.empty() = pre-weld form; never published.
    std::vector<MeshVertex> verts;
    std::vector<uint32_t>   indices;
    std::vector<MeshSubmesh> submeshes;
    std::vector<MeshSubmesh> lod_sub[KH_LOD_MAX];
    uint32_t lod_istart[KH_LOD_MAX] = {};
    uint32_t lod_icount[KH_LOD_MAX] = {};
    uint8_t  lod_n = 0;
    // Builtins are authored at unit size.
    float native_size[3] = { 1.0f, 1.0f, 1.0f };
    // KH_CLOTH: per-vertex simulation weight, parallel to verts, 0 = fully
    // pinned and 255 = fully simulated. Resolved from the kh_cloth_sim /
    // kh_cloth_pin vertex groups (FBX skin clusters) or the colour sets of the
    // same names. EMPTY means this mesh carries no cloth authoring at all,
    // which is every builtin and every model that was never painted - the
    // distinction matters, because an all-zero array is a fully pinned cloth
    // and an empty one is not cloth. A vertex only a decimated LOD level uses
    // takes the weight of the source position it sits on, else 0; none is
    // simulated (the cloth builds from level 0, and a cloth is lod_locked).
    std::vector<uint8_t> cloth_w;
    // KH_SKEL. native_ctr / native_ext are the box the importer normalized the
    // positions by, in ENGINE axes and metres: a stored position p was authored
    // at p * native_ext + native_ctr. That authored frame is the one a skeletal
    // binding puts on the parent object's origin. Builtins keep the unit box at
    // the origin. skin_bones is the skeleton - every bone a vertex group names,
    // plus its bone ancestors - and skin_inf the four strongest influences per
    // vertex, parallel to verts. skin_inf EMPTY means the mesh has no vertex
    // groups. A vertex only a decimated LOD level uses carries the influences
    // of the source position it sits on, else the blend its collapses carried
    // to the point the decimator placed it at (KH_LOD_SKIN): every level skins.
    float native_ctr[3] = { 0.0f, 0.0f, 0.0f };
    float native_ext[3] = { 1.0f, 1.0f, 1.0f };
    std::vector<KhSkinBone> skin_bones;
    std::vector<KhSkinInf> skin_inf;
    // KH_OCC_BOX (kh_mesh_occ_prep): the occluder box KH_VIS_OCC draws for this
    // mesh - normalised, engine axes, centre occ_c, half extents occ_h - valid
    // while occ_ok; and every level's vertex bounds, made whatever the proof
    // decides (KH_OCC_TARGET; the unit box until made).
    bool  occ_ok = false;
    float occ_c[3] = { 0.0f, 0.0f, 0.0f };
    float occ_h[3] = { 0.0f, 0.0f, 0.0f };
    float occ_hmn[3] = { -0.5f, -0.5f, -0.5f };
    float occ_hmx[3] = { 0.5f, 0.5f, 0.5f };
};

// One simulated point. Render vertices map onto these many-to-one: a UV seam
// splits a render vertex without splitting the cloth.
struct KhClothPart {
    // Positions in the simulation's frame: engine axes, metres, the world less
    // the instance's floating origin (kh_cloth_sync) - never world-absolute.
    float p[3];      // Current.
    float pp[3];     // At the start of the substep (Verlet's previous).
    float rest[3];   // Mesh-local rest position, the rigid-follow target.
    float w;         // Inverse mass. 0 = pinned, 1 / mass everywhere else.
    // Compliance (N^-1 m) of the attachment holding this particle at its
    // rigid-follow rest position: larger is looser, and ZERO means no
    // attachment at all. Zero would otherwise read as 'welded', which is the
    // dangerous way round for a value a zero-initialised particle takes by
    // default - and it costs nothing to rule out, because the only paint
    // weight that maps to zero compliance is the fully pinned one, whose
    // inverse mass is zero and which the attachment pass skips anyway. It is
    // the whole of the pin/sim gradient - see kh_cloth_build for the mapping,
    // and why inverse mass carries none of it.
    float att;
    // KH_CLOTH long-range attachment. 'anchor' is the nearest pinned particle
    // measured ALONG the mesh, and 'tether' is that path's rest length. A
    // per-edge stretch limit has to propagate a correction one edge per pass,
    // so on a long chain it cannot enforce anything in a single sweep; a bound
    // measured straight to a fixed anchor enforces globally in one pass and is
    // what actually stops a yanked cloth from becoming string.
    uint32_t anchor;
    float    tether;   // <= 0 = no anchor reachable (a cloth with no pins at all).
};

// One simulated face. Carried only so wind knows which way a patch of cloth is
// facing: a sheet edge-on to the wind catches nothing, and without a face the
// solver has no orientation at all, only points.
struct KhClothTri {
    uint32_t a, b, c;
};

// A distance constraint. Stretch edges and bend pairs share the type; only the
// compliance differs, so one solver loop serves both.
struct KhClothDist {
    uint32_t a, b;
    float    rest;   // Metric rest length (m at native size).
};

// Flat AABB BVH over a collider mesh's level-0 triangles, in MESH-LOCAL
// normalized space. Built once per mesh id and shared by every collider
// instance using it - the geometry never changes, only the transform does.
struct KhPhysicsBvhNode {
    float    mn[3];
    float    mx[3];
    uint32_t left;    // Interior: child index. Leaf: first triangle in tri[].
    uint32_t count;   // 0 = interior node.
};

struct KhPhysicsBvh {
    std::vector<KhPhysicsBvhNode> nodes;
    // Indices of the KEPT triangles into vp / pn (which hold them in stream
    // order), permuted by the build into the tree's leaf order. Dropped: an
    // index out of range, a non-finite corner, and a triangle with no area.
    std::vector<uint32_t>       tri;
    std::vector<float>          vp;     // Triangle corner positions, 9 floats per triangle, tri-indexed.
    // Per triangle, 18 floats: the pseudonormals of corners a, b, c, then of
    // edges ab, bc, ca - see kh_physics_bvh_build. Same indexing as vp.
    std::vector<float>          pn;
    // KH_SKEL: a DEFORMING collider's motion - each corner's displacement from
    // the previous pose to this one, 9 floats per triangle like vp, in the
    // same space. A view that deforms (KhPhysicsColliderView::deform) queries
    // the corners at vp + s * vd for s in [-1, 0], and the node boxes then
    // hold both poses. Empty = rigid, and every reader takes vp alone.
    std::vector<float>          vd;
    // KH_COL_CONVEX: the mesh as convex PIECES - its position-welded connected
    // components - for a collider the script marked convex. A particle's
    // distance to a piece is the largest of its planes' (kh_physics_convex_one).
    // Built with the BVH (kh_physics_convex_build), refitted with it
    // (kh_physics_convex_refit); the planes are in vp's space.
    std::vector<uint32_t> cvx_ptri;    // Per kept triangle (vp order - the kept stream): its piece.
    std::vector<uint32_t> cvx_pstart;  // Per piece: its first plane; pieces + 1 entries.
    std::vector<uint32_t> cvx_tri;     // Per plane: the triangle it was taken from (vp index).
    std::vector<float>    cvx_pl;      // Per plane 4: unit outward normal n, offset d (n . x = d on it), this pose.
    std::vector<float>    cvx_pl0;     // ...at the previous pose (vp - vd); empty = rigid, as vd.
    std::vector<float>    cvx_pv;      // Per plane 3: its triangle's mean corner motion (vd); empty with cvx_pl0.
    std::vector<float>    cvx_sph;     // Per piece 4: bounding sphere centre and radius, both poses.
    // The pose was refitted for the planes alone (kh_physics_bvh_refit's planes
    // arm): the root box holds, but the nodes below it and pn are the
    // template's. kh_physics_collide_one takes the convex path on such a pose
    // whatever the view asks.
    bool cvx_only = false;
    // KH_COL_CLEAR: this pose's identity (unique in the process, never 0 once
    // built or refitted), the identity of the pose vd sweeps FROM (0 = none:
    // no vd, or a chain that was cut), and the largest corner sweep |vd| of
    // the pose, in vp's units - what bounds how far any point of the surface
    // moves between two sweep fractions, or from the previous pose to this.
    uint32_t pose_seq = 0u;
    uint32_t prev_seq = 0u;
    float    vd_max = 0.0f;
    bool empty() const { return nodes.empty() || tri.empty(); }
    bool cvx_valid() const { return cvx_pstart.size() >= 2u && cvx_pl.size() == cvx_tri.size() * 4u && !cvx_tri.empty(); }
};

// A rigid frame - centre, rotation rows (engine axes, rows as basis vectors)
// and edge lengths, exactly what kh_cloth_l2w applies. What carries the cloth,
// and what carries each collider.
struct KhClothFrame {
    float centre[3];
    float rot[9];
    float size[3];
    bool  rotated;
};

// A collider, as the sim thread sees it for one frame: the shared BVH plus
// this instance's transform, snapshotted by the game thread. The BVH lives in
// the collider mesh's normalized local space times 'bake' (kh_physics_bvh_shape);
// l2w / w2l carry the rest of the size, the rotation and the centre. Every
// size here is in metres - kh_physics_collide_one divides the bake out itself.
struct KhPhysicsColliderView {
    const KhPhysicsBvh* bvh;
    // The proportions the BVH was built at. Zero means none, i.e. 1 - the
    // BVH is the plain normalized mesh.
    float bake[3];
    float rot[9];        // The collider object's rot_m, engine axes, rows as basis vectors.
    float centre[3];     // Centre: world in the gather's views, the cloth's own frame in its copies (kh_cloth_sync).
    float res[3] = {};   // KH_POS_RES: a world view's centre residual (engine axes); zero in the frame copies.
    float size[3];       // Engine-axes edge lengths; the normalized mesh spans [-0.5, 0.5].
    float inv_size[3];
    float bs_c[3];       // Bounding sphere, the broadphase - in the same frame as centre.
    float bs_r;
    float margin;        // Surface offset (m) added to the cloth's own thickness.
    float friction;      // The collider's half of the contact's Coulomb coefficient (0..1).
    bool  rotated;       // False = identity, skip the matrix entirely.
    bool  inside;        // True keeps cloth INSIDE the collider (a bag) instead of outside.
    bool  convex;        // KH_COL_CONVEX: the script marked the collider convex (kh_physics_convex_one).
    // The pose one step EARLIER: the previous frame's on the view the game
    // thread hands a cloth, the substep start's on the copy kh_cloth_step
    // makes for each substep. All-zero (a size of zero) means none - the
    // collider is taken as still.
    KhClothFrame prev;
    uint32_t slot;       // The collider object's scene slot and mesh id: what matches a view to
    int      mesh;       // the one this cloth saw last frame.
    // KH_SKEL: the BVH is a skeletal binding's skinned pose (kh_skin_run), in
    // metres about the pose box's centre, which sits at skin_ctr in the
    // model's authored frame (engine axes). kh_cloth_sync reads these to move
    // the previous pose rigidly with the root; the core never does.
    bool     skinned;
    float    skin_ctr[3];
    // KH_SKEL: the BVH's own motion this frame (its vd) applies. def0 / def1
    // are how much of it stands behind the substep's start and end (the
    // substep's fractions less one, kh_cloth_step): the corners are at
    // vp + def * vd. Zero, and unread, on a view that does not deform.
    bool     deform;
    float    def0;
    float    def1;
    // The centre's velocity (m/s) over the cloth's last step, for kh_cloth_sync's
    // teleport prediction; zero on a view seen for the first time. Game-side
    // only - the core never reads it.
    float    vel[3];
    // KH_SKIN_COL_PREV: where the centre sits from the object's root, in the
    // model's frame (engine axes) - a skinned view's BVH box centre (skin_ctr),
    // a skeletal binding's rest mesh's own centre (its skel_ctr), zero for
    // anything else. kh_cloth_sync's previous-pose rebuild takes last frame's
    // root from it. Game-side only.
    float    root_off[3] = { 0.0f, 0.0f, 0.0f };
};
struct KhClothGround {
    bool on = false;
    float x0 = 0.0f, z0 = 0.0f;       // Node (0, 0): engine axes 0 (east) and 2 (north), the instance's frame.
    float cell = 1.0f;                // Node spacing (m).
    uint32_t nx = 0, nz = 0;          // Nodes east / north.
    uint32_t px = 0, pz = 0;          // Parity of node (0, 0)'s indices in the whole matrix: the split's class.
    // By (i & 1) | (j & 1) << 1 of a cell's absolute indices: 0 = the diagonal
    // from (i, j) to (i + 1, j + 1), 1 = the one from (i + 1, j) to (i, j + 1),
    // 2 = bilinear.
    uint8_t split[4] = { 2, 2, 2, 2 };
    std::vector<float> h;             // nx * nz heights on the up axis, the instance's frame; row j = north.
};

// Per-object tuning. Everything is either a metric quantity or a normalized
// 0..1 dial - no raw compliance values reach the script surface.
struct KhClothParams {
    float stretch;        // 0..1 dial; 1 = inextensible.
    float bend;           // 0..1 dial; 0 = rag, 1 = card.
    // 0..1 MATERIAL damping per KH_CLOTH_H: it removes that fraction of the
    // rate at which each constraint stretches or folds, and nothing else. A
    // rigid motion changes no constraint, so a swing, a spin and the carrier's
    // own motion are all left to air - 'drag' and the face term.
    float damping;
    float drag;           // 0..1 air drag against absolute velocity, per KH_CLOTH_H.
    float mass;           // Particle mass (kg). Scales response to gravity vs constraints.
    float gravity;        // Multiplier on standard gravity; 0 = weightless.
    float max_stretch;    // Hard post-solve limit as a multiple of rest length (>= 1).
    float thickness;      // Cloth collision radius (m).
    float friction;       // Cloth-side half of the contact's Coulomb coefficient (0..1).
    // Per-particle speed clamp (m/s), against the CARRIER's motion: a particle
    // faster than this in the world AND relative to the carrier where it is
    // is held to this much faster than the carrier - a flag on a vehicle
    // travels at the vehicle's speed, and one the vehicle leaves is not pulled.
    float max_speed;
    // A carrier (or a collider, kh_cloth_sync) whose centre lands further than
    // this (m) from where its own velocity would have taken it in one frame
    // jumped; fast travel is motion.
    float teleport;
    // Collider and affector reach past the cloth's own extent (m; 0 reads as
    // the default, 1): the larger
    // of its rest box and how far it actually reached at its last step.
    float range;
    float sleep;          // Mean particle speed under this parks the instance's velocity (m/s).
    // THICK CLOTH. A solidified mesh - front, back, rim - is two separate
    // sheets to the position weld (front and back share no edge except the
    // rim). This is the largest gap (m) across which two OPPOSING surfaces are
    // bound together, which makes a shell behave as one piece. Set it a little
    // above the mesh's own thickness; 0 = off, right for single-sided cloth
    // and free there.
    float shell;
    // Multiplier on the world wind the game reports; 0 = this object ignores
    // the world's wind, and the air around it is still air - which still
    // resists its motion. Wind acts through the particle's own facing, so a
    // cloth edge-on to it is barely pushed, and it scales as 1 / mass.
    float wind;
    // Upward acceleration underwater as a multiple of STANDARD gravity (not of
    // the object's 'gravity' dial): 0 sinks exactly as it would in air, and at
    // gravity 1, 1 is neutrally buoyant and holds its depth, above 1 rises. A
    // cloth settles where buoyancy balances gravity, so 'gravity'
    // divided by this is the fraction of KH_CLOTH_WATER_BAND it floats at.
    float buoyancy;
    // Velocity damping per KH_CLOTH_H while submerged, the twin of 'drag' and
    // ramped in over the same band. Water is not air: a cape entering it
    // should slow hard rather than keep swinging.
    float water_drag;
    // SELF-COLLISION (clothSimulation's selfCollision / selfThickness /
    // selfFriction keys), off by default. With it on, no particle comes
    // nearer than self_thickness to a face of the same cloth, from the side
    // it approached, and no edge nearer than that to another edge - or, for
    // a pair already nearer in the rest pose along the surface's normal,
    // nearer than it was there, so an authored fold or a shell's two
    // surfaces keep their own gap. self_friction is the Coulomb coefficient
    // between the two layers; 'friction' is the cloth's half of a COLLIDER
    // contact and is not read here. See kh_cloth_self_gather.
    bool  self_collide;
    float self_thickness;   // Separation held between layers (m).
    float self_friction;    // Coulomb coefficient between layers (0..1).
    uint16_t iterations;  // Constraint iterations per substep.
    // Substeps per 1/60 s, NOT per frame - the substep length is fixed and
    // must not follow the frame rate (see kh_cloth_step). A slow frame runs
    // more of them and a fast frame fewer. The backlog one step may simulate
    // is bounded in TIME (KH_CLOTH_DT_MAX), never in substeps, so this dial
    // sets how finely the solve runs and not the frame rate below which the
    // cloth runs slow: that floor is the stall clamp's, 10 fps, at every
    // setting.
    uint16_t substeps;
};
struct KhColClear {        // One particle against one collider column. d <= 0 = nothing known.
    float l[3];            // Where the particle stood, the collider's model frame.
    float d;               // Its distance from the surface then (the BVH's units), found outside.
    float acc;             // The column's surface-motion sum then.
};
struct KhColClearCol {     // One collider column's pose chain.
    uint32_t seq = 0u;     // The pose last used...
    float    s = 0.0f;     // ...at this sweep fraction (def1; 0 = the pose itself)...
    float    vd_max = 0.0f;   // ...and its largest corner sweep.
    float    acc = 0.0f;   // Surface motion summed over the chain (the BVH's units).
    bool     on = false;   // The column's records may be read and written.
};

// Everything one instance needs to step, and nothing that resizes while a
// worker holds it. The particle array is sized once at build; 'stretch' is
// re-sized only by kh_cloth_build_shell, on the game thread and only on an
// instance no worker holds; the solver's own arrays are sized on the worker.
// So a reader off the worker sees a stale value at worst, never a freed one.
// The self-collision pairs (kh_cloth_self_gather): a particle against a face
// of its own cloth, and an edge against an edge, that can reach each other
// this substep.
struct KhClothSelfC {
    uint32_t q;      // The particle.
    uint32_t t;      // The face, an index into KhClothState::tri.
    float    side;   // +1 / -1: the side of the face's plane the particle is on (started on, until it goes round).
    float    sep;    // The separation held: self_thickness, or the pair's rest distance if nearer.
    float    dep;    // Separation restored so far this substep (m) - the friction cone's normal term.
    float    s0;     // Its distance from the face's plane at the substep's start (m, >= 0).
};
struct KhClothSelfEdge {
    uint32_t a, b;   // A surface edge's particles.
    uint32_t f;      // A face it borders, whose normal says which way the surface faces there.
};
struct KhClothSelfE {
    uint32_t e0, e1;   // Indices into KhClothState::self_edges, e0 < e1.
    float    n[3];     // Unit direction from e1's nearest point to e0's: the start's, until they go round.
    float    sep;
    float    dep;
    float    d0;       // The nearest points' distance at the substep's start (m).
};

struct KhClothState {
    std::vector<KhClothPart> part;
    // Mesh edges, then the shell links appended after them (see
    // kh_cloth_build_shell). They are all distance constraints, so one set
    // serves both: the shell links take the same compliance, ride the same
    // per-edge stretch limit - which is what bounds how far a thickness may
    // change - and join the tether graph, where a path through the shell is a
    // real path.
    std::vector<KhClothDist> stretch;
    std::vector<KhClothDist> bend;      // Opposite corners across a shared edge.
    std::vector<KhClothTri>  tri;       // Level-0 faces: the wind normal and self-collision's faces.
    float    centre[3];                 // The frame the rest pose was last resolved against.
    float    rot[9];
    float    size[3];
    bool     rotated;
    bool     primed;                    // False until the first step has placed the particles.
    // Sea level on the up axis of the frame the particles live in. The core
    // is frame-agnostic and this is its one absolute fact; the runtime
    // simulates about a floating origin and sets it. Zero is 'the frame is
    // the world', the right answer for a zero-initialised state.
    float    sea;
    float    accum;                     // Substep accumulator (s).
    // KH_SIM_LOD: the substep length the particles' displacement (p - pp)
    // was last integrated over; 0 = none yet. A step at another length
    // rescales the displacement first (kh_cloth_step), because the
    // integrator reads it back as the velocity over ONE substep.
    float    h_last;
    float    speed_mean;                // Mean particle speed at the last step (m/s), the sleep test.
    // XPBD Lagrange multipliers, one per constraint, reset at the start of
    // every substep. Sized there to their sets, and re-sized there whenever a
    // set's size changes (a rebuilt shell changes stretch's). Worker-private:
    // nothing outside kh_cloth_substep reads them, so they are not part of the
    // published state.
    std::vector<float> lam_stretch;
    std::vector<float> lam_bend;
    std::vector<float> lam_att;
    // Wind's facing, rebuilt each substep: 7 floats per particle, the sum over
    // the faces around it of |n| nhat nhat^T (xx xy xz yy yz zz) and then of
    // |n| - their ratio is the area-weighted mean of nhat nhat^T.
    std::vector<float> nrm;
    // SELF-COLLISION, rebuilt on every substep that has it on. Worker-private
    // scratch like the multipliers above, resized as the gather needs.
    std::vector<KhClothSelfC> self_c;   // This substep's particle / face pairs.
    std::vector<KhClothSelfE> self_e;   // This substep's edge / edge pairs.
    std::vector<KhClothSelfEdge> self_edges;   // The surface's edges, built once per face set.
    uint32_t              self_edges_nt;   // The face count self_edges was built from; zero = never.
    std::vector<float>    self_rest;    // Metric rest positions, 3 per particle: the rest separations.
    float                 self_rest_size[3];   // The size self_rest was built at.
    float                 self_edge;    // Mean metric rest edge length, the hash cell's floor.
    std::vector<float>    self_box;     // Per face, then per edge: its box over the substep (6 floats).
    std::vector<uint32_t> self_wide;    // KH_CLOTH_SELF_WIDE: this substep's wide elements (usually none).
    std::vector<uint32_t> self_head;    // Hash bucket starts, one past the table.
    std::vector<uint32_t> self_ent;     // Element indices (self_box's: faces, then edges), bucket by bucket.
    std::vector<uint8_t>  self_tang;    // Per particle: 1 = at an edge-through-face intersection at the substep's start.
    // KH_CLOTH_FORK: the gather's queries run in chunks, across the pool when
    // the cloth is large (kh_cloth_fork_run). Per participant (the job's own
    // worker and each helper): its own 'seen' stamps (per element, the query
    // that last met it, plus one) and tangle marks; per chunk: the pairs it
    // listed, concatenated in chunk order afterwards.
    // Worker-private scratch like the rest; a helper touches only its own
    // participant's and its chunk's entries.
    std::vector<std::vector<uint32_t>>     self_seen_par;
    std::vector<std::vector<uint8_t>>      self_tang_par;
    std::vector<std::vector<KhClothSelfC>> self_c_par;
    std::vector<std::vector<KhClothSelfE>> self_e_par;
    // The jolt stand-down (kh_cloth_step): seconds left with self-collision
    // off, and the velocity of each corner of the carrier's box last frame.
    float                 self_hold;
    float                 self_v[8][3];
    bool                  self_vok;        // self_v holds a frame; false = no velocity yet.
    // The carrier centre's velocity over the last frame (m/s), for the
    // teleport test's prediction (kh_cloth_step). carrier_vok false = none
    // yet, which is the plain distance test. Recorded on a rebased frame too:
    // otherwise a carrier past the teleport distance per frame would rebase
    // for ever, never having a velocity to predict with.
    float                 carrier_v[3];
    bool                  carrier_vok;
    // KH_TELEPORT_ONCE: carrier_v was taken across a rebase (the jump's own
    // travel), so the next test also takes no jump from where the carrier stood.
    bool                  carrier_vreb;
    // How far (m) the farthest particle stood from the carrier's centre at the
    // end of the last step, or 0 before one: kh_cloth_sync's reach for colliders and
    // affectors. A scalar, written by the worker, read on a non-busy instance.
    float                 reach;
    // Worker-private: the per-substep collider views (kh_cloth_step).
    std::vector<KhPhysicsColliderView> col_sub;
    // KH_CLOTH_WARM: per collider (in col_sub's order) per particle, the
    // triangle its last query hit (KH_CLOTH_WARM_NONE = none), and per
    // collider the slot / mesh the column was filled for - a column whose
    // collider changed identity starts over. Worker-private.
    std::vector<uint32_t> col_warm;
    std::vector<uint32_t> col_warm_key;
    std::vector<KhColClear>    col_clear;       // KH_COL_CLEAR: col_warm's layout, a record per particle per column.
    std::vector<KhColClearCol> col_clear_col;   // ...and each column's pose chain.
    // KH_SKEL: a skeletal binding's guide for the rigid-follow targets
    // (kh_cloth_target), 3 floats per particle, mesh-local like 'rest': where
    // the skeleton puts each particle at the start of the frame (a) and at its
    // end (b). A simulated particle's entry is its rest position - the bones
    // move only what is pinned, wholly or in part. Both empty = no guide, and
    // every target is 'rest'. Filled on the worker ahead of the step
    // (kh_cloth_guide_prep). guide_t0 / guide_t1 are the fractions of the
    // frame the running substep starts and ends at, so guide_t1 is where the
    // frame in 'centre' stands (kh_cloth_step sets both).
    std::vector<float>    guide_a;
    std::vector<float>    guide_b;
    float                 guide_t0;
    float                 guide_t1;
};
// KH_INJ_FORK: fn(i) for every i in [0, n), in chunks across the pool with
// the caller helping (kh_cloth_fork_run: one fork at a time; a fork already
// up, no workers, or a small n runs the plain loop). The contract is the
// fork's: fn reads only what is frozen for the call and writes only its own
// index's slot; it takes no lock and touches no device object. Chunk order is
// unspecified, so nothing fn writes may depend on another index.
static constexpr uint32_t KH_FORK_EACH_MIN = 512u;   // Below this the fork's wake costs more than it saves.

// KH_AFFECTOR - PHYSICS AFFECTORS (addPhysicsAffector), the core's half.
//
// A scripted field that pushes cloth in one of three ways, and the way is what
// separates the types:
// - AIR (wind, turbulence, vortex) moves the air. It adds to the world wind at
//   the particle and acts through exactly that term: through the particle's
//   own facing, against its own motion, as 1 / mass, and not under water - so
//   cloth edge-on to it is barely pushed, and the cloth's 'wind' dial, which
//   scales only the world's wind, does not scale it.
// - FORCE (force, radial) accelerates every simulated particle alike, as
//   gravity does, whatever its facing or mass.
// - DRAG (drag) thickens the medium: a further fraction of the velocity is
//   lost per KH_CLOTH_H, on top of the cloth's own drag and stated the same way.
// Each is confined to an ellipsoid in its own axes (or acts everywhere),
// faded to zero across the outer 'edge' fraction of it, and scaled by its
// lifetime envelope. kh_cloth_sync builds the views in the instance's own
// frame; the core only evaluates them, and with none the step is the step it
// was before affectors existed, bit for bit.
static constexpr uint32_t KH_AFF_MODES = 8u;       // Turbulence's Fourier modes.

// One affector as a cloth step sees it: engine axes, in the instance's frame.
// p is the type's parameters as kh_aff_frame_build resolved them - wind:
// speed (m/s); turbulence: unused (its modes carry it); vortex: swirl,
// inflow, updraft (m/s), core (m, > 0); force and radial: strength (m/s^2);
// drag: strength (0..1 per KH_CLOTH_H).
struct KhClothAffView {
    uint8_t type;
    bool    bounded;                 // False = acts everywhere; inv_r unread.
    float   c[3];                    // Centre.
    float   ax[9];                   // Rows: right, forward (the script's vectorDir), up.
    float   inv_r[3];                // 1 / half-extent along each row.
    float   edge;                    // Outer fraction of the ellipsoid faded to zero (0 = hard edge).
    float   gain;                    // Lifetime envelope this frame (0..1).
    float   p[4];
    // Turbulence: each mode's wave vector (rad/m), amplitude (m/s, already
    // perpendicular to the wave vector), angular frequency (rad/s) and phase
    // at the END of this frame (rad, wrapped to one turn).
    float   mk[KH_AFF_MODES][3];
    float   ma[KH_AFF_MODES][3];
    float   mw[KH_AFF_MODES];
    float   mph[KH_AFF_MODES];
};

// KH_CHAIN - BONE-DRIVEN RIGID BODIES: a rope, a chain, an ammo belt. The
// mesh's own skeleton is the simulation: every bone from the chain's START
// bone down is a rigid body (the start itself kinematic, pinned where the
// carrier or the bound skeleton puts it), each joined to its parent at its
// own head, and the mesh is skinned from those bodies exactly as a skeletal
// binding skins from its proxies. What collides is the GEOMETRY: every
// level-0 vertex whose strongest influence is a simulated bone is a contact
// point on that bone's body, against the terrain and every physics collider,
// so a flat belt lands flat and a chain drapes over what it touches. The END
// bone may be held too - its head at a position, its orientation at a
// rotation - each followed, fixed or free (KhChainTarget).
//
// THE SOLVER is extended position-based dynamics for rigid bodies (Mueller,
// Macklin, Chentanez, Jeschke, Kim 2020): per substep, integrate each free
// body, then project the joints (position at the shared head; swing and twist
// against the rest orientation, each with its own stiffness and optional
// limit), the end's hold and the contacts - positional corrections applied at
// a point through each body's generalised inverse mass - then take the
// velocities from the motion and damp the joints' relative spin. The substep
// is the cloth's, fixed at 1 / (60 * substeps) s, with the cloth's backlog and
// stall rules (kh_cloth_step), and the carrier, the skeleton and the colliders
// are interpolated across the substeps the same way.
//
// FRAMES. The rest pose lives in CARRIER-LOCAL metres: engine axes about the
// rest box's centre (the mesh's native_ctr), the object drawn at its authored
// size - a simulated chain ignores the size property, as a skeletal binding
// does, because the bones' lengths are the model's. Every body frame IS the
// carrier-local frame at rest, so the rest relative rotation of every joint is
// the identity. The state lives in the instance's floating-origin frame like
// the cloth's particles. A body's orientation is a unit quaternion (x, y, z,
// w) taking body vectors to that frame.
struct KhChainParams {
    float stretch;        // 0..1: the joints' positional stiffness; 1 (the default) holds them exactly.
    float bend;           // 0..1: swing stiffness toward the rest angle; 0 = limp rope, 1 = rigid.
    float twist;          // 0..1: twist stiffness about each bone's axis.
    float damping;        // 0..1 of the joints' relative spin removed per KH_CLOTH_H.
    // 0..1 of a body's motion RELATIVE TO ITS CARRIER removed per KH_CLOTH_H -
    // the loss 'damping' cannot give, because a chain swinging as one has
    // almost no relative spin between its links for that dial to act on.
    // Relative to the carrier, not to the world, so a belt on a moving vehicle
    // is damped toward travelling WITH it and never lags; on a still carrier
    // the two are the same thing. 0 = off, and the step is then bit-identical
    // to one without these dials.
    float lin_damping;
    float ang_damping;    // ...and of its spin relative to the carrier's own turn.
    float drag;           // 0..1 of the velocity against the air (the world wind x wind) removed per KH_CLOTH_H.
    float mass;           // kg, the whole simulated chain, shared by each bone's length.
    float gravity;        // Multiplier on standard gravity.
    float thickness;      // Contact offset (m) held between the geometry and the ground or a collider.
    float friction;       // The chain's half of a contact's Coulomb coefficient (0..1).
    float swing_limit;    // Degrees a joint may bend from its rest angle; 180 = no limit.
    float twist_limit;    // Degrees a joint may twist from its rest angle; 180 = no limit.
    float max_speed;      // m/s against the carrier, the cloth's guard (KhClothParams::max_speed).
    float teleport;       // As the cloth's: a carrier or collider missing its predicted place by this (m) jumped.
    float range;          // As the cloth's: collider and affector reach past the chain's own (m; 0 reads as 1).
    float sleep;          // Mean body speed (m/s) under which a chain on a still carrier parks.
    float wind;           // Multiplier on the world wind the game reports.
    float buoyancy;       // Upward acceleration underwater as a multiple of standard gravity (the cloth's rule).
    float water_drag;     // The twin of 'drag' while submerged.
    uint16_t iterations;  // Constraint passes per substep.
    uint16_t substeps;    // Substeps per 1/60 s, as the cloth's.
};

// One simulated bone. The rest members are carrier-local metres and, for the
// vectors that belong to the body, its own frame (the carrier-local axes at
// rest); the state members are the instance's frame.
struct KhChainBody {
    float x0[3];          // Rest centre of mass.
    float inv_m;          // 0 = kinematic (the start bone).
    float ib_inv[9];      // Inverse inertia about the centre of mass, body frame, symmetric.
    float ib[9];          // ...and the inertia itself (kh_chain_joint_solve's mass block).
    float radius;         // Farthest contact point from the centre of mass (m): the broadphase.
    int32_t bone;         // MeshDef::skin_bones index.
    int32_t parent;       // Parent body, or -1 for the start.
    uint32_t s0, sn;      // Its contact points: KhChainState::samp[s0 .. s0 + sn).
    float x[3], q[4];     // Now.
    float v[3], w[3];     // Linear (m/s) and angular (rad/s) velocity.
    float xp[3], qp[4];   // At the substep's start.
};
// A joint at the child's head: ra / rb the head from each body's centre of
// mass in its own frame; u the twist axis and e a vector across it, in the
// child's frame (the parent's too - the rest relative rotation is the
// identity).
struct KhChainJoint {
    uint32_t a, b;
    float ra[3], rb[3];
    float u[3], e[3];
};
// What the end is held by this frame, in the instance's frame. Kinds: 0 free;
// 1 the bone's own place on the carrier (the bound skeleton's pose, or its
// authored place in the mesh's frame); 2 this position / these axes.
struct KhChainTarget {
    uint8_t pos = 0;
    uint8_t rot = 0;
    bool    pos_hold = false;   // Kind 2 comes from the chain's own pose when it first stood (false).
    bool    rot_hold = false;
    float   p[3] = { 0.0f, 0.0f, 0.0f };
    float   r[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };   // Rows: the images of the body axes.
};
struct KhChainState {
    std::vector<KhChainBody>  body;    // Parents before children; body 0 is the start.
    std::vector<KhChainJoint> joint;   // Parents before children, one per body but the start.
    std::vector<float>        samp;    // 3 per contact point: its offset from its body's centre of mass, body frame.
    std::vector<int32_t>      body_of; // Per bone: its body, or -1 (the bone follows the carrier or the skeleton).
    int32_t end_body = -1;             // The end bone's body, or -1 (none named and none implied).
    // KH_CHAIN_POINT - which point of the START and END bones the chain is
    // anchored and held at (the script's startPoint / endPoint). A bone's HEAD
    // is its own origin; its TAIL is the far end - its first simulated child's
    // head, or, on a tip bone with no child to end it, its own geometry's
    // reach down the joint's axis.
    // end_pt: the point the end's hold acts on, in the END body's frame. The
    // default is that bone's head, which IS joint[end_body - 1].rb.
    float   end_pt[3] = { 0.0f, 0.0f, 0.0f };
    // start_off: 3 per body, the carrier-local translation the REST placement
    // takes, so the pose the joints want is the pose the reset lays down.
    // Empty with startPoint "tail" (the default), where the anchor already IS
    // the rest pose's - again bit-identical. With "head" the free subtree is
    // placed one start bone shorter, which is what moving the anchor to the
    // head means; without it the first step would close that gap as a snap and
    // read it back as a frame of velocity.
    std::vector<float> start_off;
    float   auth_ctr[3] = { 0.0f, 0.0f, 0.0f };   // MeshDef::native_ctr: carrier-local = authored - this.
    // Skinning, immutable after the build: the worker reads these, never the MeshDef.
    std::vector<KhSkinInf> inf;        // Per render vertex, or empty.
    std::vector<uint8_t>   lvl0;       // Per render vertex: 1 = level 0 draws it (the box), or empty = all.
    float ext[3] = { 1.0f, 1.0f, 1.0f };   // MeshDef::native_ext: a stored position times this is carrier-local.
    // The skeleton's pose at the last step's end (kh_skin_affine's layout), the
    // start of this step's interpolation. Empty = none.
    std::vector<float> guide_prev;
    // The end's targets at the last step's end, the start of this step's
    // interpolation of a kind-2 target (the kind it was, so a change is not
    // swept).
    uint8_t prev_pos = 0, prev_rot = 0;
    float   prev_p[3] = { 0.0f, 0.0f, 0.0f };
    float   prev_q[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // A 'false' target: the end's pose when the chain first stood, held for
    // the instance's life (the floating origin carries it: kh_cloth_origin_shift).
    bool    hold_on = false;
    float   hold_p[3] = { 0.0f, 0.0f, 0.0f };
    float   hold_q[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // How far the end's held point can get from where the chain hangs off the
    // start (the bone lengths along the path, plus the end bone's own length
    // when endPoint holds its tail - KH_CHAIN_POINT), and the body whose joint
    // is that pin. A position target further away than this cannot be met
    // together with the joints (both are rigid rows) and the solve would
    // alternate between them forever; a real inextensible chain goes taut
    // instead, which is what clamping the target to this radius gives. The
    // path exists when end_root > 0 (KH_CHAIN_REACH_ZERO): its reach may be 0 -
    // an end bone hanging straight off the start, held at its head, IS the pin.
    float   end_reach = 0.0f;
    int32_t end_root = -1;
    // The end's APPROACH: where a hold engages away from where the end stands
    // (a target set across the map, a bone the chain cannot reach), the gap is
    // closed at KH_CHAIN_SEEK_MS / KH_CHAIN_SEEK_RAD rather than in one
    // substep, since a correction taken in one substep is read back as that
    // distance over one substep of velocity. seek_p / seek_q are what is still
    // to close: added to the target, shrinking every substep.
    bool    seek_p_on = false;
    bool    seek_q_on = false;
    float   seek_p[3] = { 0.0f, 0.0f, 0.0f };
    float   seek_q[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
    // Worker-private: 6 multipliers per joint then 6 for the end's hold, reset
    // per substep, and the direct solve's scratch (kh_chain_joint_solve).
    std::vector<float> lam;
    // Which of each joint's three rotation rows stood at its limit on the last
    // pass (3 per joint). A row's multiplier belongs to the constraint it was
    // accumulated against, so it is dropped whenever that changes.
    std::vector<uint8_t> lam_lim;
    std::vector<double> solve_wk;
    std::vector<int32_t> solve_meta;
    std::vector<int32_t> solve_at;
    float    mass_built = 0.0f;        // The 'mass' the bodies were distributed at.
};

struct KhMaterialMap {
    std::string path;   // Resolved disk path ("" = slot empty).
    // KH_USER_TEX: the colour space a user slot is sampled in (the script's
    // "srgb"; default linear). The builtin slots take kh_tex_slot_srgb's rule
    // and never set it.
    bool srgb = false;
};

// Map slots per material: the builtin six (pages bound at t14-t18 and t42),
// then KH_USER_TEX's user0..user5 (slots 6-11, a .hlsl material only; pages
// bound at t43-t48, read by the user shader alone through KhUserTex).
static constexpr int KH_MAT_MAPS_BUILTIN = 6;
static constexpr int KH_MAT_USER_MAPS = 6;
static constexpr int KH_MAT_MAPS = KH_MAT_MAPS_BUILTIN + KH_MAT_USER_MAPS;

// KH_BIND_FILTER - what kh_bind_material and kh_dlr_append last bound (the material pages at t14 - t18, t42 and
// t43 - t48, the material table at t38; the light ring at t40), so a bind of the views a slot already holds is
// skipped. Materials share their pages by texture class (KH_MAT_PAGES) and the table and the ring are one view
// each, so consecutive submesh draws mostly rebound identical pointers: four runtime calls per submesh, one per lit
// object. A skipped call would have bound what the slot holds - the pipeline is exactly as it was. The record is
// believed only inside a KhBindScope (opened after the state capture of the injection, the flush, the PIP, the
// in-front slice and seam, the sun's union and tiers, the DLS faces, the seams and the dlsw mask; its thread and
// context; it begins and ends knowing nothing) and is forgotten by every other writer of those slots:
// StateBackup::restore (t0 - t48), KhRpSave's
// restore (t0 - t63), the replay's draws (t0 - t15), mask_cast_engine's restore (t0 - t32) and kh_fx_tex_bind /
// unbind (t43 - t48) - every other PSSetShaderResources in the renderer binds other slots, and none of these views is
// ever a render target or UAV (whose bind would unbind it). A recorded view is bound, so the context's reference
// keeps it alive: no new view can take its address while the record names it. Outside a scope - or on another
// context - both functions bind as they always did. Per thread: a pass and its scope run on one thread.
struct KhBindCache {
    ID3D11DeviceContext* ctx = nullptr;   // The outermost scope's context; null outside every scope.
    int depth = 0;                        // Open scopes on this thread.
    bool k14 = false, k42 = false, k43 = false, k38 = false, k40 = false;   // The range's views are known.
    ID3D11ShaderResourceView* v14[5] = {};
    ID3D11ShaderResourceView* v42 = nullptr;
    ID3D11ShaderResourceView* v43[KH_MAT_USER_MAPS] = {};
    ID3D11ShaderResourceView* v38 = nullptr;
    ID3D11ShaderResourceView* v40 = nullptr;
};
static thread_local KhBindCache t_kh_bc;
inline void kh_bc_forget() noexcept {
    t_kh_bc.k14 = t_kh_bc.k42 = t_kh_bc.k43 = t_kh_bc.k38 = t_kh_bc.k40 = false;
}
inline bool kh_bc_on(const ID3D11DeviceContext* khbo_ctx) noexcept {
    return t_kh_bc.depth > 0 && t_kh_bc.ctx == khbo_ctx;
}
struct KhBindScope {
    explicit KhBindScope(ID3D11DeviceContext* khbs_ctx) noexcept {
        if (t_kh_bc.depth++ == 0) t_kh_bc.ctx = khbs_ctx;
        kh_bc_forget();
    }
    ~KhBindScope() {
        kh_bc_forget();
        if (--t_kh_bc.depth == 0) t_kh_bc.ctx = nullptr;
    }
    KhBindScope(const KhBindScope&) = delete;
    KhBindScope& operator=(const KhBindScope&) = delete;
};

// KH_UV_ANIM - a material slot's texture-coordinate animation, the script's uv* / flipbook params (every kind:
// pbr, arma, .hlsl). It moves every map of the slot together - the builtin six and the user maps - and every reader
// of them: the colour twins, the coverage verdicts and every alpha caster (KhMatUv, cb.hlsl). The lookup is
//     uv' = S * R(rotation + rotationSpeed * t) * (uv - 0.5) + 0.5 + offset + scroll * t
// then the warp (a ripple across each axis, tied to the surface), then the flipbook cell; t is the session clock -
// objects sharing a material animate in step. Defaults leave the uv as it is.
struct KhUvAnim {
    float scale[2] = { 1.0f, 1.0f };    // "uvScale": repeats per uv unit (negative mirrors).
    float offset[2] = { 0.0f, 0.0f };   // "uvOffset".
    float scroll[2] = { 0.0f, 0.0f };   // "uvScroll": uv units per second.
    float rot = 0.0f;                   // "uvRotation": degrees, about the tile's centre (0.5, 0.5).
    float rot_speed = 0.0f;             // "uvRotationSpeed": degrees per second.
    float warp[3] = { 0.0f, 0.0f, 0.0f };   // "uvWarp": amplitude (uv units), frequency (per uv unit), speed (/s).
    float flip[4] = { 1.0f, 1.0f, 0.0f, 1.0f };   // "flipbook": columns, rows, frames per second, frames.
};

struct KhMaterial {
    bool used = false;   // Slot carries an assignment.
    int shader = 0;   // 0 = "pbr"; 1 = custom user.hlsl (user_shader); 2 = "arma" (builtin).
    std::string user_shader;   // Resolved.hlsl path when shader == 1 ("" otherwise).
    // Diffuse, normal, orm, emissive, specular, speccolor. The arma model keeps
    // its AS map in the orm slot and its SMDI map in the specular slot, and
    // takes speccolor; a .hlsl material takes all six under either naming
    // (KH_USER_SLOTS, map_as / map_smdi below).
    KhMaterialMap maps[KH_MAT_MAPS];
    float base_color[3] = { 1.0f, 1.0f, 1.0f };
    float roughness = 0.8f;
    float metalness = 0.0f;
    float emissive_intensity = 1.0f;
    float normal_strength = 1.0f;
    float cutoff = 0.5f;
    int alpha_mode = 0;   // No material mode touches casting: an object casts as a whole.
    // User channel routes (slot*4+chan; -1 = slot-convention default)
    int route_occ = -1, route_rough = -1, route_metal = -1;
    int route_alpha = -1, route_gloss = -1;
    // The arma model. Its defaults make a map-less arma material shade as a
    // map-less pbr one: fresnel (1.5, 0) is 0.04 head-on, pbr's dielectric F0,
    // and glossiness 0.2 is pbr's roughness 0.8.
    int   route_spec = -1;                            // Specular intensity (default SMDI.g).
    float spec_color[3] = { 1.0f, 1.0f, 1.0f };       // Tints the specular (x SPECCOLOR).
    float glossiness = 0.2f;                          // Gloss with no SMDI map (0 matte - 1 mirror).
    float fresnel_n = 1.5f;                           // Refractive index N.
    float fresnel_k = 0.0f;                           // Absorption coefficient K.
    // KH_USER_SLOTS - a .hlsl material may give its orm slot as "as" and its
    // specular slot as "smdi" (the arma model's maps: the same two slots). The
    // name says what the slot holds: SMDI's channels are data, so the specular
    // slot is then read linear (kh_tex_map_srgb), and the default routes are
    // the named map's own (kh_mat_gpu_fill). False for "pbr"; unread for
    // "arma", whose two slots hold AS and SMDI by definition.
    bool map_as = false;
    bool map_smdi = false;
    // KH_USER_VS - the .hlsl defines KhUserVertex (kh_user_vertex_scan, at the
    // parse): its vertices are deformed by it, once a cycle (kh_uvs_step).
    // vertex_bound is the script's promise of how far it moves any vertex (m,
    // "vertexBound"): the CPU cannot see the deformation, so every bounds
    // test pads by it (kh_bounds_size_of). vertex_static ("vertexStatic") says
    // its result depends on nothing that changes by itself - no clock, no
    // camera - so it is evaluated again only when its inputs move.
    bool  user_vertex = false;
    float vertex_bound = 0.0f;
    bool  vertex_static = false;
    // KH_UV_ANIM: the animation and what the parse derived from it (kh_uv_anim_derive): uv_anim = any of it differs
    // from the defaults, uv_anim_time = some of it moves with the clock.
    KhUvAnim uv;
    bool uv_anim = false;
    bool uv_anim_time = false;
};

struct KhMaterialSet {
    std::vector<KhMaterial> slots;
    bool any = false;   // At least one used slot.
    // KH_USER_VS: some slot has a vertex stage; the largest vertex_bound among
    // them; every one of them is vertex_static. Derived at the parse.
    bool  vertex_any = false;
    float vertex_bound = 0.0f;
    bool  vertex_static = false;
    // KH_UV_ANIM: some used slot with an alpha mode and an alpha source (a routed alpha, or a diffuse map) animates
    // with the clock - its casters' shadow and seam keys then change every cycle (kh_mat_anim_cycle_key). Derived at
    // the parse.
    bool  uv_anim_alpha = false;
    // Content hash (kh_material_set_hash): the KH_MAT_POOL intern key and the
    // instancing batch key. Covers every field kh_bind_material and
    // kh_draw_textured read.
    uint64_t hash = 0;
    // KH_MAT_TABLE: the set's GPU state, filled by kh_mat_gpu_ensure at first
    // draw and reset by the table rebuild; mutable because the set is shared
    // and const to its holders. Written by the render thread or a park only.
    // KH_MAT_GPU_NOCOPY: a copy of a set starts with none of it and never reads
    // it - the game thread copies a live pool set (kh_apply_material_update's
    // copy-on-write base) while the render thread may be filling this very
    // state (pages.assign), and the copy's state was discarded anyway
    // (kh_material_intern resets it).
    struct Gpu {
        int base = -1;   // First table entry (one per slot); -1 = not uploaded.
        uint64_t page_sig = 0;   // Hash over the pages every slot's maps live in:
                                 // the material half of the instancing batch key.
        struct PageSet { ID3D11ShaderResourceView* p[KH_MAT_MAPS]; };
        std::vector<PageSet> pages;   // Per slot, t14-t18, t42 and t43-t48.
        Gpu() = default;
        Gpu(const Gpu&) {}
        Gpu& operator=(const Gpu&) {
            base = -1;
            page_sig = 0;
            pages.clear();
            return *this;
        }
    };
    mutable Gpu gpu;
};

// KH_FX_TEX: an effect's own textures (the script's "textures", user0 .. user5) as one immutable, interned
// set - its paths (interned, nullptr = none) and bit i = user<i> is colour (sRGB) - so an object carries one
// pointer (RenderObject::fx_tex, nullptr = no textures) rather than the set. Interned for the process as the
// paths are (one per distinct set; never released), on the game thread (the SQF parser); readers dereference
// an immutable record.
struct KhFxTexSet {
    const std::string* path[6] = {};
    uint8_t srgb = 0;
};

// A sound key: depth_srv holds a reference on the texture, so its address
// cannot be reused while the SRV lives; nulled wherever depth_srv is released.
// Written by ensure_depth_srv from one thread at a time (a park's flush, the
// render thread, or a chain-only flush inside a game-thread resolve).
static void* g_eds_memo_res = nullptr;

// KH_DL_SHADOW slice budget. 8 lights x 6 cube faces is the ceiling; the array
// is grown to demand, never allocated to the ceiling (see dls_slices).
static constexpr int KH_DLS_SLICES_MAX = 8 * 6;

// KH_SUN_LADDER: the sun ladder's validity flags, tier keys and input hash
// describe maps and pyramids Resources owns, so Resources::release drops them
// itself rather than leaving it to the caller - ensure_resources releases on
// its own failure paths and does not call release_shadow_device_state. Declared
// here and not with the prototypes below Resources, because release() calls it
// from an inline member body. Defined in rendering_integration_features.hpp;
// release_shadow_device_state, its only other caller, is in rendering_integration_injection.hpp.
inline void kh_sun_ladder_forget();

// KH_GLOW_PYR - one glow pass's pyramid (cb.hlsl's KhGlowTap reads it, effect.hlsl's PSGlowSeed / PSGlowDown make it;
// kh_glow_build). Level 0 is half the frame, padded up to a multiple of 2^(KH_GLOW_LEVELS - 1) so every level halves
// exactly and one uv addresses them all; R16G16B16A16_FLOAT, for the scene's HDR bright passes. Resources holds one
// per route that draws glows (glow_pyr): [0] the scene flush - its effect meshes and then its chain, one after the
// other - [1] the UI pass, [2] the PIP. Each is made at its first use, remade when its frame changes size, and
// released with the device (Resources::release).
static constexpr int KH_GLOW_LEVELS = 8;   // Texels of 2 .. 256 frame pixels.
struct KhGlowPyr {
    ID3D11Texture2D*          tex = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;   // Every level: the pass's taps (t3).
    ID3D11RenderTargetView*   rtv[KH_GLOW_LEVELS] = {};
    ID3D11ShaderResourceView* lvl_srv[KH_GLOW_LEVELS] = {};   // One level each: the next level's source.
    UINT                      w = 0, h = 0;   // Level 0.
    void release() {
        for (int khgp_i = 0; khgp_i < KH_GLOW_LEVELS; ++khgp_i) {
            KH_SAFE_RELEASE(rtv[khgp_i]);
            KH_SAFE_RELEASE(lvl_srv[khgp_i]);
        }
        KH_SAFE_RELEASE(srv);
        KH_SAFE_RELEASE(tex);
        w = h = 0;
    }
};
struct KhAnaPyr {
    ID3D11Texture2D*          tex[2] = {};
    ID3D11ShaderResourceView* srv[2] = {};   // The pass's taps (t3 / t6); the other level's source while building.
    ID3D11RenderTargetView*   rtv[2] = {};
    UINT                      fw = 0, fh = 0;   // The frame it is made for.
    void release() {
        for (int khap_i = 0; khap_i < 2; ++khap_i) {
            KH_SAFE_RELEASE(rtv[khap_i]);
            KH_SAFE_RELEASE(srv[khap_i]);
            KH_SAFE_RELEASE(tex[khap_i]);
        }
        fw = fh = 0;
    }
};

// Forward: KH_THM_DEVRESET - the heightfield's live flags, dropped by Resources::release (defined after
// kh_thm_upload in rendering_integration_features.hpp).
inline void kh_thm_device_lost();

struct Resources {
    ID3D11VertexShader*      vs = nullptr;
    ID3D11PixelShader*       ps = nullptr;
    ID3D11VertexShader*      vs_fullscreen = nullptr;   // SV_VertexID fullscreen triangle.
    ID3D11PixelShader*       ps_effect = nullptr;   // Uber effect shader (meshes + fullscreen).
    UINT                     ps_effect_samples = 0;   // Depth MSAA count it was compiled for.
    // KH_FX_SPLIT / KH_FX_LINZ (effect3.hlsl's tail): the effect unit's chain-side entry points - the linear-depth
    // targets' builder, the SSGI gather and a-trous, dynamicLightFog's gather and filter - made with ps_effect at its
    // sample count (ensure_effect_shader); each consumer gates on its pointer.
    ID3D11PixelShader*       ps_fx_linz = nullptr;
    ID3D11PixelShader*       ps_ssgi_gather = nullptr;
    ID3D11PixelShader*       ps_ssgi_atrous = nullptr;
    ID3D11PixelShader*       ps_dlf_gather = nullptr;
    ID3D11PixelShader*       ps_dlf_filter = nullptr;
    ID3D11VertexShader*      vs_composite = nullptr;   // injected-path VS (adds world position).
    ID3D11PixelShader*       ps_maskcast = nullptr;   // Analytic mask cast (single-sample t0).
    ID3D11PixelShader*       ps_rpmerge = nullptr;    // KH_VOL_REPLAY: the replayed count and footprint, merged.
    ID3D11PixelShader*       ps_rpmergemir = nullptr;   // KH_MIR_REPLAY: the replayed mirror count, merged.
    ID3D11PixelShader*       ps_dls_world = nullptr;   // KH_DLS_WORLD: dynamic-light world receive
                                                       // (single-sample t0).
    ID3D11PixelShader*       ps_dls_world_fog = nullptr;   // KH_DLSW_FOG: its dual-source twin.
    ID3D11PixelShader*       ps_seam_foot = nullptr;   // KH_VOL_FOOT: the footprint's distance.
    ID3D11PixelShader*       ps_maskprime = nullptr;   // Writes lit over our footprint.
    ID3D11PixelShader*       ps_inj_depth_a = nullptr;   // KH_FOOTPRINT_ALPHA: the clip-only alpha
                                                         // Footprint (KH_TEXTURED).
    ID3D11PixelShader*       ps_composite = nullptr;   // PSMain + the opaque.
    ID3D11PixelShader*       ps_composite_arb = nullptr;   // Arb variant: + SV_Depth analytic
                                                           // Terrain clamp.
    UINT                     ps_composite_samples = 0;   // comp_depth_samples it was compiled for (always 1; 0 = none).
    ID3D11Texture2D*          comp_depth_tex = nullptr;   // Owned mid-frame copy of the scene
                                                          // Depth.
    ID3D11ShaderResourceView* comp_depth_srv = nullptr;   // The live depth is bound as a writable
                                                          // DSV during injection - sampling it
                                                          // directly is a bind hazard.
    UINT                     comp_depth_w = 0, comp_depth_h = 0, comp_depth_samples = 1;
    float                    comp_depth_time = -1.0f;   // Last snapshot copy (overlay freshness
                                                        // Gate).
    ID3D11RenderTargetView*   snap_rtv = nullptr;   // RTV over comp_depth_tex.
    ID3D11PixelShader*        ps_depth_resolve = nullptr;
    UINT                      ps_resolve_samples = 0;   // Source MSAA count compiled for.
    // Analytic terrain heightfield (t10; see thmParams).
    ID3D11Texture2D*          thm_tex = nullptr;
    ID3D11ShaderResourceView* thm_srv = nullptr;
    ID3D11InputLayout*       input_layout = nullptr;
    // Deliberately its own shaders, layout, buffers and states: everything else
    // ensure_resources builds sits below the async gate the placeholder exists
    // to escape.
    ID3D11VertexShader*      vs_white = nullptr;
    ID3D11PixelShader*       ps_white = nullptr;
    ID3D11InputLayout*       layout_white = nullptr;
    ID3D11Buffer*            white_obj_cb = nullptr;   // b0, CBObj slice.
    ID3D11Buffer*            white_frame_cb = nullptr;   // b1, CBFrame slice.
    // Same bias, same DepthClipEnable false, so the placeholder sits where the
    // real mesh will.
    ID3D11RasterizerState*   rast_white = nullptr;
    ID3D11DepthStencilState* dss_white = nullptr;   // LESS_EQUAL, write all.
    bool                     white_tried = false;   // One compile attempt per device.
    bool                     white_ready = false;
    ID3D11RasterizerState*   rasterizer_cull = nullptr;   // back-face-culling twin of 'rasterizer'
                                                          // (twoSided = false objects; same
                                                          // biases).
    // front-culling twin: pass 1 of the two-sided-translucent ordered draw.
    // Below: the KH_TEXTURED twins, the same sources compiled with
    // KH_TEXTURED=1.
    ID3D11RasterizerState*   rasterizer_front = nullptr;
    ID3D11VertexShader*      vs_tex = nullptr;   // VSMain twin (flush).
    ID3D11PixelShader*       ps_tex = nullptr;   // PSMain twin.
    ID3D11VertexShader*      vs_comp_tex = nullptr;   // VSComposite twin (injection).
    ID3D11PixelShader*       ps_comp_tex = nullptr;   // PSComposite twin.
    ID3D11PixelShader*       ps_comp_arb_tex = nullptr;   // Arb twin.
    ID3D11InputLayout*       layout_tex = nullptr;
    ID3D11VertexShader*      vs_inst = nullptr;
    ID3D11VertexShader*      vs_inst_tex = nullptr;
    ID3D11VertexShader*      vs_comp_inst = nullptr;
    ID3D11VertexShader*      vs_comp_inst_tex = nullptr;
    ID3D11InputLayout*       layout_inst = nullptr;
    ID3D11InputLayout*       layout_inst_tex = nullptr;
    // KH_SKIN_POOL: the vertex-pulling twins (static.hlsl / composite2.hlsl
    // *Sk) - the colour buckets' four (the composite pair is the composite
    // unit's) and the depth runs' three (an alpha caster draws one instance
    // at a time, through the IA route) - and their one layout (VSInstSk: the
    // lane alone, on slot 1). Non-fatal: the colour buckets take their four
    // whole (kh_inst_sk_on), a depth route its own twin (kh_cast_pool_of).
    ID3D11VertexShader*      vs_inst_sk = nullptr;
    ID3D11VertexShader*      vs_inst_sk_tex = nullptr;
    ID3D11VertexShader*      vs_comp_inst_sk = nullptr;
    ID3D11VertexShader*      vs_comp_inst_sk_tex = nullptr;
    ID3D11VertexShader*      vs_sundepth_sk = nullptr;
    ID3D11VertexShader*      vs_seam_inst_sk = nullptr;
    ID3D11VertexShader*      vs_mirror_inst_sk = nullptr;
    ID3D11InputLayout*       layout_inst_sk = nullptr;
    ID3D11Buffer*            inst_vb[2] = {};
    uint32_t                 inst_cursor[2] = {};
    bool                     inst_fresh[2] = { true, true };   // Next map must discard.
    ID3D11SamplerState*      mat_sampler = nullptr;
    ID3D11Buffer*             mat_sb = nullptr;   // KH_MAT_TABLE: StructuredBuffer<KhGpuMat>, t38.
    ID3D11ShaderResourceView* mat_srv = nullptr;
    uint32_t                  mat_cap = 0;   // Entries the buffer holds.
    // KH_OBJBUF: the persistent object record buffer (StructuredBuffer<KhObjRec>,
    // VS t39), one record per live-scene slot, DEFAULT usage, delta-updated by
    // kh_objbuf_sync from the CPU mirror (UpdateSubresource per dirty run).
    ID3D11Buffer*             obj_sb = nullptr;
    ID3D11ShaderResourceView* obj_srv = nullptr;
    uint32_t                  obj_cap = 0;   // Records the buffer holds.
    // Release wall: a freed mesh's VB/IB/def wait two flushes so per-frame
    // caster lists built before the release drain first. Swept by
    // kh_mesh_grave_sweep.
    struct KhMeshGrave { ID3D11Buffer* vb; ID3D11Buffer* ib; MeshDef* def; uint64_t flush_at; };
    std::vector<KhMeshGrave>   mesh_grave;
    std::vector<ID3D11Buffer*> mesh_ib;   // KH_MESH_INDEXED: the R32_UINT index buffer twin of
                                          // mesh_vb, same id, same lifetime (nullptr = tombstone).
    std::vector<ID3D11Buffer*> mesh_vb;   // One VB per published registry mesh (48-byte textured
                                          // Vertex format; grows in step via ensure_mesh_vbs - see
                                          // the dynamic-registration contract).
    ID3D11Buffer*            constant_buffer = nullptr;   // Dynamic, per draw (the scene and UI flushes):
                                                          // CBObj b0.
    ID3D11Buffer*            composite_cb = nullptr;   // Dynamic, per draw (render-thread
                                                       // Injection): CBObj b0.
    // KH_CB_RING: the per-object slices of the two buffers above ride a large
    // dynamic ring each - WRITE_DISCARD once per wrap, WRITE_ NO_OVERWRITE per
    // slice, bound with the D3D11.1 offset setters.
    ID3D11Buffer*            cb_ring[2] = {};   // [0] = constant_buffer's ring, [1] =
                                                // composite_cb's.
    uint32_t                 cb_ring_cursor[2] = {};
    bool                     cb_ring_fresh[2] = { true, true };   // Next map must discard.
    // CB-split frame twins (b1): the flushes' and the injection's, the same split as the pair above.
    ID3D11Buffer*            frame_cb = nullptr;   // The scene flush + the UI flush.
    ID3D11Buffer*            composite_frame_cb = nullptr;   // Render thread (injection, sun depth,
                                                             // Mask cast).
    ID3D11DepthStencilState* dss_test = nullptr;   // LESS_EQUAL, no write.
    ID3D11DepthStencilState* dss_test_write = nullptr;   // LESS_EQUAL, write.
    // KH_SSAO_MARK: dss_test_write plus stencil = ref (1) where the depth test
    // passes (our pixels, for the AO pass); the eraser (depth off, stencil
    // EQUAL -> ZERO); the apply's mask (depth off, stencil EQUAL, no writes;
    // null = no depth view).
    ID3D11DepthStencilState* dss_test_write_mark = nullptr;
    ID3D11DepthStencilState* dss_mark_clear = nullptr;
    ID3D11DepthStencilState* dss_mark_test = nullptr;
    ID3D11DepthStencilState* dss_off = nullptr;
    ID3D11BlendState*        blend_modes[6] = {};   // Normal, additive, multiply, screen, lighten, darken.
    ID3D11BlendState*        dls_world_blend = nullptr;
    ID3D11BlendState*        dls_world_fog_blend = nullptr;   // KH_DLSW_FOG: src0 + dest * src1.
    // World-pass occlusion mask: R32_FLOAT linear view distance (m) of our
    // nearest mesh per pixel; min_blend resolves overlaps, no depth target.
    ID3D11Texture2D*          dlsw_tex = nullptr;
    ID3D11RenderTargetView*   dlsw_rtv = nullptr;
    ID3D11ShaderResourceView* dlsw_srv = nullptr;
    UINT                      dlsw_w = 0, dlsw_h = 0;
    ID3D11BlendState*         dlsw_min_blend = nullptr;   // Dest = min(src, dest).
    ID3D11VertexShader*       vs_dls_mask = nullptr;
    ID3D11PixelShader*        ps_dls_mask = nullptr;
    ID3D11PixelShader*        ps_dls_mask_a = nullptr;   // KH_DLSW_MASK_ALPHA: the alpha-clipping twin.
                                                         // Also KH_VOL_FOOT's alpha casters (the seam).
    // KH_SSAO: the four pass shaders (at the live depth's sample count), the
    // half grid's sample-0 depth in metres (R32_FLOAT), the term and its
    // ping-pong twin (R8_UNORM), and the pass's 80-byte b0 (KhSsaoCb).
    ID3D11PixelShader*        ps_ssao_depth = nullptr;
    ID3D11PixelShader*        ps_ssao_main = nullptr;
    ID3D11PixelShader*        ps_ssao_blur = nullptr;
    ID3D11PixelShader*        ps_ssao_apply = nullptr;
    UINT                      ps_ssao_samples = 0;
    ID3D11Texture2D*          ssao_dh_tex = nullptr;
    ID3D11RenderTargetView*   ssao_dh_rtv = nullptr;
    ID3D11ShaderResourceView* ssao_dh_srv = nullptr;
    ID3D11Texture2D*          ssao_ao_tex = nullptr;
    ID3D11RenderTargetView*   ssao_ao_rtv = nullptr;
    ID3D11ShaderResourceView* ssao_ao_srv = nullptr;
    ID3D11Texture2D*          ssao_ao2_tex = nullptr;
    ID3D11RenderTargetView*   ssao_ao2_rtv = nullptr;
    ID3D11ShaderResourceView* ssao_ao2_srv = nullptr;
    UINT                      ssao_w = 0, ssao_h = 0;     // The main depth's size the targets were made for.
    UINT                      ssao_hw = 0, ssao_hh = 0;   // The half grid's.
    ID3D11Buffer*             ssao_cb = nullptr;
    // KH_NEARZ_MARK: the near-plane marker (R32G32_FLOAT, the main depth's size: our near-z pixels' raw depth and
    // true distance), made on first need; its pass shader (ssao.hlsl).
    ID3D11PixelShader*        ps_ssao_nzmark = nullptr;
    ID3D11Texture2D*          nzm_tex = nullptr;
    ID3D11RenderTargetView*   nzm_rtv = nullptr;
    ID3D11ShaderResourceView* nzm_srv = nullptr;
    UINT                      nzm_w = 0, nzm_h = 0;
    // KH_VM_SEE: the scene's depth behind the view model's translucent planes (R32_FLOAT: sample 0 of the main depth,
    // raw, at the cycle's first view-model translucent draw) and their transmittance (R8_UNORM: cleared to 1 there,
    // times 1 - alpha per view-model draw that blends over with it), both the main depth's size, made by the first
    // snapshot that needs them (kh_vmsee_targets) and released when the main depth goes to one sample
    // (kh_msaa_toggle_wipe: no late chain reads them there); the snapshot's pass shader (depth_resolve.hlsl's
    // PSDepthS0, at the main depth's sample count) and the transmittance's blend (ZERO / INV_SRC_ALPHA, red only),
    // made at the arm (kh_vmsee_ensure). A refused make is not retried for the same size (targets) or count (shader)
    // until the device goes.
    ID3D11Texture2D*          vmsee_s_tex = nullptr;
    ID3D11RenderTargetView*   vmsee_s_rtv = nullptr;
    ID3D11ShaderResourceView* vmsee_s_srv = nullptr;
    ID3D11Texture2D*          vmsee_t_tex = nullptr;
    ID3D11RenderTargetView*   vmsee_t_rtv = nullptr;
    ID3D11ShaderResourceView* vmsee_t_srv = nullptr;
    UINT                      vmsee_w = 0, vmsee_h = 0;
    UINT                      vmsee_fail_w = 0, vmsee_fail_h = 0;
    ID3D11PixelShader*        ps_vmsee_s0 = nullptr;
    UINT                      ps_vmsee_samples = 0;
    UINT                      ps_vmsee_fail = 0;
    ID3D11BlendState*         bs_vmsee_t = nullptr;
    bool                      bs_vmsee_failed = false;
    // KH_PIP_FX: the PIP pass's own scene capture (t0, copied per pass), its
    // depth copied (t1; single-sample, so PSEffect's MSAA_DEPTH 0 build reads
    // it), and that build. Sized to the PIP the last fire saw.
    ID3D11PixelShader*        ps_effect_pip = nullptr;
    bool                      ps_effect_pip_tried = false;   // One compile attempt per device.
    ID3D11Texture2D*          pip_scene_tex = nullptr;
    ID3D11ShaderResourceView* pip_scene_srv = nullptr;
    // The chain's second buffer: passes ping-pong [0] -> [1] -> [0] and the
    // last one writes the PIP target, so the PIP is copied once per cycle.
    ID3D11Texture2D*          pip_scene_tex2 = nullptr;
    ID3D11ShaderResourceView* pip_scene_srv2 = nullptr;
    ID3D11RenderTargetView*   pip_scene_rtv[2] = {};
    UINT                      pip_scene_w = 0, pip_scene_h = 0;
    DXGI_FORMAT               pip_scene_fmt = DXGI_FORMAT_UNKNOWN;
    ID3D11Texture2D*          pip_depth_tex = nullptr;
    ID3D11ShaderResourceView* pip_depth_srv = nullptr;
    UINT                      pip_depth_w = 0, pip_depth_h = 0;
    DXGI_FORMAT               pip_depth_fmt = DXGI_FORMAT_UNKNOWN;
    ID3D11BlendState*        blend_ui_replace = nullptr;   // Blend off, RGB-only write: builtin
                                                           // UI-phase.
    // Custom user FX only: hardware-lerps the chain-composited output by
    // coverage (their shaders cannot be assumed coverage-aware).
    ID3D11BlendState*        blend_ui_masked = nullptr;
    // Custom user FX only, a pass with uiSpill (the builtin spill lane
    // composites in-shader): the output plus the frame's uncovered share,
    // src + dst x (1 - dst.a).
    ID3D11BlendState*        blend_ui_spill = nullptr;
    ID3D11BlendState*        blend_alpha_zero = nullptr;   // Blend off, alpha-only write mask.
    ID3D11PixelShader*       ps_alpha_zero = nullptr;
    ID3D11Texture2D*          cov_hist_tex = nullptr;   // last-sane coverage history (full).
    // Backbuffer copy taken post-correction: by induction its alpha is always
    // sane; the substitute mask for poison frames.
    ID3D11ShaderResourceView* cov_hist_srv = nullptr;
    UINT                      cov_hist_w = 0;
    UINT                      cov_hist_h = 0;
    uint32_t                  cov_hist_fmt = 0;
    ID3D11PixelShader*       ps_cov_fix = nullptr;
    ID3D11PixelShader*       ps_premult_copy = nullptr;
    ID3D11RasterizerState*   rasterizer = nullptr;   // cullNone, MSAA, no depth bias, no depth clip.
    ID3D11Texture2D*          sun_tex = nullptr;
    ID3D11DepthStencilView*   sun_dsv = nullptr;
    ID3D11ShaderResourceView* sun_srv = nullptr;
    ID3D11Texture2D* sun2_tex = nullptr;
    ID3D11DepthStencilView* sun2_dsv = nullptr;
    ID3D11ShaderResourceView* sun2_srv = nullptr;
    ID3D11Texture2D* sun3_tex = nullptr;
    ID3D11DepthStencilView* sun3_dsv = nullptr;
    ID3D11ShaderResourceView* sun3_srv = nullptr;
    ID3D11Texture2D* sun4_tex = nullptr;
    ID3D11DepthStencilView* sun4_dsv = nullptr;
    ID3D11ShaderResourceView* sun4_srv = nullptr;
    ID3D11Texture2D* sun5_tex = nullptr;   // Far band (KH_SUN_FAR_BAND).
    ID3D11DepthStencilView* sun5_dsv = nullptr;
    ID3D11ShaderResourceView* sun5_srv = nullptr;
    // [0]=hero [1]=mid [2]=outer.
    ID3D11Texture2D*          sun_pf_tex[4] = {};   // KH_FAR_PF: [3] = the far tier's.
    ID3D11RenderTargetView*   sun_pf_rtv[4] = {};
    ID3D11ShaderResourceView* sun_pf_srv[4] = {};
    ID3D11RenderTargetView*   sun_pf_rtvm[4][12] = {};
    ID3D11Texture2D*          sun_pf_scr = nullptr;
    ID3D11ShaderResourceView* sun_pf_scr_srvm[12] = {};
    ID3D11PixelShader*        ps_sunpfm = nullptr;
    ID3D11BlendState*         blend_pf = nullptr;
    ID3D11SamplerState*       samp_pf = nullptr;   // linear-clamp; s1 (KH_SHADOW_GATHER).
    ID3D11VertexShader*       vs_sunpf = nullptr;
    ID3D11PixelShader*        ps_sunpf = nullptr;
    // KH_SKIN_GPU (kh_skin_so_ensure): VSSkinSo, the geometry-stage object that
    // carries its stream-output declaration, the rest stream's layout and the
    // palette's constant buffer.
    ID3D11VertexShader*       skin_so_vs = nullptr;
    ID3D11GeometryShader*     skin_so_gs = nullptr;
    ID3D11InputLayout*        skin_so_layout = nullptr;
    ID3D11Buffer*             skin_so_cb = nullptr;
    ID3D11Buffer*             uvs_obj_cb = nullptr;     // KH_USER_VS: the deform pass's own b0 / b1 (the white
    ID3D11Buffer*             uvs_frame_cb = nullptr;   // placeholder's no-borrowing contract).
    ID3D11RasterizerState*    rast_sun = nullptr;   // cullNone, no scene depth bias.
    // rast_sun's twin at a lower bias (CullNone, as rast_sun).
    ID3D11RasterizerState*    rast_sun_lo = nullptr;   // low-bias default (8 / 1.0).
    ID3D11RasterizerState*    rast_sun_pk = nullptr;
    ID3D11RasterizerState*    rast_sun_lo_pk = nullptr;
    // The injection'S own rasterizer.
    ID3D11RasterizerState*    rast_inject = nullptr;
    ID3D11VertexShader*       vs_sundepth = nullptr;
    ID3D11InputLayout*        layout_sundepth = nullptr;   // Mesh slot 0 + per-instance slot 1.
    // KH_CAST_ALPHA: the alpha-aware twins (the same instanced transform with
    // the uv, a depth-only PS that clips on the caster's alpha).
    ID3D11VertexShader*       vs_sundepth_a = nullptr;
    ID3D11PixelShader*        ps_sundepth_a = nullptr;
    ID3D11InputLayout*        layout_sundepth_a = nullptr;   // TEXCOORD0 (uv) on slot 0.
    ID3D11VertexShader*       vs_sundepth_ao = nullptr;      // KH_CAST_ALPHA: VSSunDepthAObj (KH_SUN_OBJ).
    ID3D11InputLayout*        layout_sundepth_ao = nullptr;  // Its layout: the mesh's own four lanes, slot 0 only.
    // One depth array for every casting light: a spot spends 1 slice, a point
    // spends 6 (a cube unrolled).
    ID3D11RasterizerState*    dls_rast = nullptr;
    ID3D11Texture2D*          dls_tex = nullptr;
    ID3D11ShaderResourceView* dls_srv = nullptr;
    ID3D11DepthStencilView*   dls_dsv[KH_DLS_SLICES_MAX] = {};
    UINT                      dls_slices = 0;
    UINT                      dls_size = 0;
    ID3D11Buffer*             sun_instance_vb = nullptr;
    UINT                      sun_instance_cap = 0;   // Instances the buffer holds.
    // Seam transport instanced twins; both take layout_sundepth's lane shape.
    // Non-fatal: absent, both loops draw per caster.
    ID3D11VertexShader*       vs_seam_inst = nullptr;     // VSSeamInst   (loop A footprint).
    ID3D11VertexShader*       vs_mirror_inst = nullptr;   // VSMirrorInst (loop B prepass).
    // One buffer, both loops: loop A's lanes at [0, nA), loop B's at
    // [nA, nA + nB), written by a single WRITE_DISCARD map per frame.
    ID3D11Buffer*             seam_instance_vb = nullptr;
    UINT                      seam_instance_cap = 0;   // Lanes the buffer holds.
    ID3D11Buffer*             locality_buf = nullptr;   // Extended (>16) caster reach list.
    ID3D11ShaderResourceView* locality_srv = nullptr;   // t2 at the mask cast fire.
    UINT                      locality_cap = 0;   // Casters the list holds.
    // KH_CAST_OCC: the caster-occupancy grid at t35. A world-XZ texture whose
    // texel holds the min/max Y of every caster reach volume covering that
    // column, so PSMaskCast's "is this point near a caster" gate is one Load
    // instead of a per-pixel loop over every caster.
    ID3D11Texture2D*          castocc_tex = nullptr;
    ID3D11ShaderResourceView* castocc_srv = nullptr;
    bool                     initialized = false;

    // Post-tonemap backbuffer capture (UI-affecting passes).
    ID3D11Texture2D*          bb_tex = nullptr;
    ID3D11ShaderResourceView* bb_srv = nullptr;
    ID3D11RenderTargetView*   bb_rtv = nullptr;   // The spill lane's premultiplying capture draws
                                                  // Into the capture texture.
    UINT                      bb_w = 0, bb_h = 0;
    DXGI_FORMAT               bb_fmt = DXGI_FORMAT_UNKNOWN;
    ID3D11Texture2D*          ui_alpha_stage = nullptr;   // 64x64 staging window for the UI alpha readback.
    ID3D11Texture2D*          ui_chain_tex = nullptr;
    ID3D11RenderTargetView*   ui_chain_rtv = nullptr;
    ID3D11ShaderResourceView* ui_chain_srv = nullptr;

    // Fullscreen-chain ping-pong targets (single-sample, scene format): one
    // scene Resolve per frame regardless of pass count.
    ID3D11Texture2D*          chain_tex[5] = {};   // [3] = a-trous sibling of [2]; [4] = fp16
                                                   // radiance seed (KH_SSGI_SEED).
    ID3D11RenderTargetView*   chain_rtv[5] = {};
    ID3D11ShaderResourceView* chain_srv[5] = {};
    // Linear-clamp sampler at s2 for the ssgi side chain (the shared prefix
    // owns s1): the radiance seed (26), the gather's radiance taps (22) and
    // the resolve's upsample (24) sample through it. Bound from the seed to
    // the resolve, the prior binding saved/restored (StateBackup covers s0-s1
    // only).
    ID3D11SamplerState*       khsg_sampler = nullptr;
    // KH_GLOW_PYR: the glow passes' pyramids (KhGlowPyr, per route), their builders (effect.hlsl PSGlowSeed /
    // PSGlowDown, made once with the core set - absent, every glow takes its direct taps) and the anisotropic clamp
    // sampler the taps read through at s2 (bound only for a glow draw, the prior binding put back: KhGlowBind).
    KhGlowPyr                 glow_pyr[3];
    ID3D11PixelShader*        ps_glow_seed = nullptr;
    ID3D11PixelShader*        ps_glow_down = nullptr;
    ID3D11SamplerState*       glow_sampler = nullptr;
    // KH_ANA_PYR: anamorphic's one-axis pyramids (KhAnaPyr, per glow_pyr route and per axis: [route][0] along x,
    // [route][1] along y) and their builders (effect.hlsl PSAnaSeed / PSAnaDown, made with the glow builders -
    // absent, anamorphic takes its direct taps). No sampler: its taps are Loads.
    KhAnaPyr                  ana_pyr[3][2];
    ID3D11PixelShader*        ps_ana_seed = nullptr;
    ID3D11PixelShader*        ps_ana_down = nullptr;
    // KH_FX_SIDE (effect.hlsl's note): a pass's side value, drawn by PSEffect itself before the pass
    // (kh_fx_side_draw) - [0] fog scatter's per-pixel fog (R16_FLOAT, the frame's size), [1] the sun flare's
    // visibility and [2] the UI lane's coverage probe (R32_FLOAT, 1 x 1). Made at first use, remade on a size
    // change, released with the device.
    ID3D11Texture2D*          fxs_tex[3] = {};
    ID3D11RenderTargetView*   fxs_rtv[3] = {};
    ID3D11ShaderResourceView* fxs_srv[3] = {};
    UINT                      fxs_w[3] = {};
    UINT                      fxs_h[3] = {};
    // KH_FX_LINZ (effect.hlsl's note): the frame's linear depth [0] and the SSGI / dynamicLightFog grids' copies [1] /
    // [2] (R32_FLOAT), drawn by kh_fx_linz_draw. Made at first use, remade on a size change, released with the device.
    ID3D11Texture2D*          linz_tex[3] = {};
    ID3D11RenderTargetView*   linz_rtv[3] = {};
    ID3D11ShaderResourceView* linz_srv[3] = {};
    UINT                      linz_w[3] = {};
    UINT                      linz_h[3] = {};
    ID3D11Texture2D*          scene_tex = nullptr;
    ID3D11ShaderResourceView* scene_srv = nullptr;
    UINT                      scene_w = 0, scene_h = 0;
    DXGI_FORMAT               scene_fmt = DXGI_FORMAT_UNKNOWN;
    // KH_BLEND_MINMAX: a lighten / darken mesh's own copy of the scene (kh_mm_capture), apart from scene_tex.
    ID3D11Texture2D*          mm_tex = nullptr;
    ID3D11ShaderResourceView* mm_srv = nullptr;
    UINT                      mm_w = 0, mm_h = 0;
    DXGI_FORMAT               mm_fmt = DXGI_FORMAT_UNKNOWN;

    // Engine depth buffer as shader resource.
    ID3D11ShaderResourceView* depth_srv = nullptr;
    // KH_SSAO_MARK: its stencil plane (uint, .g), where the format has one;
    // null on a stencil-less depth (then no AO).
    ID3D11ShaderResourceView* depth_sten_srv = nullptr;
    ID3D11DepthStencilView*   depth_dsv_ro = nullptr;   // read-only view: lets PS sample depth
                                                        // While depth-testing.
    void*                     depth_res_identity = nullptr;   // Identity only, never dereferenced.
    UINT                      depth_sample_count = 0;

    void*                    comp_depth_identity = nullptr;
    void release_bb_capture() {
        if (bb_tex) { bb_tex->Release(); bb_tex = nullptr; }
        if (bb_srv) { bb_srv->Release(); bb_srv = nullptr; }
        if (bb_rtv) { bb_rtv->Release(); bb_rtv = nullptr; }
        if (ui_alpha_stage) { ui_alpha_stage->Release(); ui_alpha_stage = nullptr; }
        KH_SAFE_RELEASE(ui_chain_tex);   // The chain twin shares the group.
        KH_SAFE_RELEASE(ui_chain_rtv);
        KH_SAFE_RELEASE(ui_chain_srv);
        bb_w = 0; bb_h = 0; bb_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_fx_chain() {
        for (int i = 0; i < 5; ++i) {   // Gather; + pre-smooth; + radiance seed.
            KH_SAFE_RELEASE(chain_tex[i]);
            KH_SAFE_RELEASE(chain_rtv[i]);
            KH_SAFE_RELEASE(chain_srv[i]);
        }
        KH_SAFE_RELEASE(khsg_sampler);
    }

    void release_scene_capture() {
        KH_SAFE_RELEASE(scene_tex);
        KH_SAFE_RELEASE(scene_srv);
        scene_w = 0; scene_h = 0; scene_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_mm_capture() {   // KH_BLEND_MINMAX.
        KH_SAFE_RELEASE(mm_srv);
        KH_SAFE_RELEASE(mm_tex);
        mm_w = 0; mm_h = 0; mm_fmt = DXGI_FORMAT_UNKNOWN;
    }

    void release_depth_srv() {
        if (depth_srv) { depth_srv->Release(); depth_srv = nullptr; }
        KH_SAFE_RELEASE(depth_sten_srv);   // KH_SSAO_MARK.
        if (depth_dsv_ro) { depth_dsv_ro->Release(); depth_dsv_ro = nullptr; }
        depth_res_identity = nullptr;
        depth_sample_count = 0;
        g_eds_memo_res = nullptr;   // The fast-path key dies with the SRV.
    }

    // KH_VM_SEE: the two targets - at a size change or a refused make (kh_vmsee_targets), a switch to one sample
    // (kh_msaa_toggle_wipe) and the device's release.
    void release_vmsee_targets() {
        KH_SAFE_RELEASE(vmsee_s_srv);
        KH_SAFE_RELEASE(vmsee_s_rtv);
        KH_SAFE_RELEASE(vmsee_s_tex);
        KH_SAFE_RELEASE(vmsee_t_srv);
        KH_SAFE_RELEASE(vmsee_t_rtv);
        KH_SAFE_RELEASE(vmsee_t_tex);
        vmsee_w = vmsee_h = 0;
    }

    void release() {
        KH_SAFE_RELEASE(vs);
        KH_SAFE_RELEASE(ps);
        KH_SAFE_RELEASE(vs_fullscreen);
        KH_SAFE_RELEASE(ps_effect);
        ps_effect_samples = 0;
        KH_SAFE_RELEASE(ps_fx_linz);   // KH_FX_SPLIT / KH_FX_LINZ.
        KH_SAFE_RELEASE(ps_ssgi_gather);
        KH_SAFE_RELEASE(ps_ssgi_atrous);
        KH_SAFE_RELEASE(ps_dlf_gather);
        KH_SAFE_RELEASE(ps_dlf_filter);
        KH_SAFE_RELEASE(vs_composite);
        KH_SAFE_RELEASE(ps_composite);
        KH_SAFE_RELEASE(ps_composite_arb);
        ps_composite_samples = 0;
        KH_SAFE_RELEASE(comp_depth_tex);
        KH_SAFE_RELEASE(comp_depth_srv);
        KH_SAFE_RELEASE(snap_rtv);
        KH_SAFE_RELEASE(ps_depth_resolve);
        ps_resolve_samples = 0;
        comp_depth_time = -1.0f;
        KH_SAFE_RELEASE(thm_tex);
        KH_SAFE_RELEASE(thm_srv);
        kh_thm_device_lost();   // KH_THM_DEVRESET: the live flags go with the texture.
        comp_depth_identity = nullptr;
        comp_depth_w = comp_depth_h = 0;
        comp_depth_samples = 1;
        KH_SAFE_RELEASE(input_layout);
        KH_SAFE_RELEASE(vs_tex);
        KH_SAFE_RELEASE(vs_inst);
        KH_SAFE_RELEASE(vs_inst_tex);
        KH_SAFE_RELEASE(vs_comp_inst);
        KH_SAFE_RELEASE(vs_comp_inst_tex);
        KH_SAFE_RELEASE(layout_inst);
        KH_SAFE_RELEASE(layout_inst_tex);
        KH_SAFE_RELEASE(vs_inst_sk);   // KH_SKIN_POOL.
        KH_SAFE_RELEASE(vs_inst_sk_tex);
        KH_SAFE_RELEASE(vs_comp_inst_sk);
        KH_SAFE_RELEASE(vs_comp_inst_sk_tex);
        KH_SAFE_RELEASE(vs_sundepth_sk);
        KH_SAFE_RELEASE(vs_seam_inst_sk);
        KH_SAFE_RELEASE(vs_mirror_inst_sk);
        KH_SAFE_RELEASE(layout_inst_sk);
        for (int khin_i = 0; khin_i < 2; ++khin_i) {
            KH_SAFE_RELEASE(inst_vb[khin_i]);
            inst_cursor[khin_i] = 0;
            inst_fresh[khin_i] = true;
        }
        KH_SAFE_RELEASE(ps_tex);
        KH_SAFE_RELEASE(vs_comp_tex);
        KH_SAFE_RELEASE(ps_comp_tex);
        KH_SAFE_RELEASE(ps_comp_arb_tex);
        KH_SAFE_RELEASE(layout_tex);
        KH_SAFE_RELEASE(mat_sampler);
        KH_SAFE_RELEASE(mat_srv);
        KH_SAFE_RELEASE(mat_sb);
        mat_cap = 0;
        KH_SAFE_RELEASE(obj_srv);
        KH_SAFE_RELEASE(obj_sb);
        obj_cap = 0;
        for (auto& khr_mb : mesh_vb) KH_SAFE_RELEASE(khr_mb);
        mesh_vb.clear();
        for (auto& khr_mi : mesh_ib) KH_SAFE_RELEASE(khr_mi);
        mesh_ib.clear();
        for (auto& khr_mg : mesh_grave) {
            KH_SAFE_RELEASE(khr_mg.vb);
            KH_SAFE_RELEASE(khr_mg.ib);
            delete khr_mg.def;
            khr_mg.def = nullptr;
        }
        mesh_grave.clear();
        KH_SAFE_RELEASE(sun_tex);
        KH_SAFE_RELEASE(sun_dsv);
        KH_SAFE_RELEASE(sun_srv);
        KH_SAFE_RELEASE(sun2_srv);
        KH_SAFE_RELEASE(sun2_dsv);
        KH_SAFE_RELEASE(sun2_tex);
        KH_SAFE_RELEASE(sun3_srv);
        KH_SAFE_RELEASE(sun3_dsv);
        KH_SAFE_RELEASE(sun3_tex);
        KH_SAFE_RELEASE(sun4_srv);
        KH_SAFE_RELEASE(sun4_dsv);
        KH_SAFE_RELEASE(sun4_tex);
        KH_SAFE_RELEASE(sun5_srv);
        KH_SAFE_RELEASE(sun5_dsv);
        KH_SAFE_RELEASE(sun5_tex);
        // Released here: nothing else owns the pyramid resources.
        for (int khpf_i = 0; khpf_i < 4; ++khpf_i) {
            KH_SAFE_RELEASE(sun_pf_srv[khpf_i]);
            KH_SAFE_RELEASE(sun_pf_rtv[khpf_i]);
            KH_SAFE_RELEASE(sun_pf_tex[khpf_i]);
            for (int khpf_m = 0; khpf_m < 12; ++khpf_m)
                KH_SAFE_RELEASE(sun_pf_rtvm[khpf_i][khpf_m]);
        }
        for (int khpf_m = 0; khpf_m < 12; ++khpf_m)
            KH_SAFE_RELEASE(sun_pf_scr_srvm[khpf_m]);
        KH_SAFE_RELEASE(sun_pf_scr);
        KH_SAFE_RELEASE(ps_sunpfm);
        KH_SAFE_RELEASE(ps_sunpf);
        KH_SAFE_RELEASE(vs_sunpf);
        KH_SAFE_RELEASE(uvs_frame_cb);   // KH_USER_VS.
        KH_SAFE_RELEASE(uvs_obj_cb);
        KH_SAFE_RELEASE(skin_so_cb);   // KH_SKIN_GPU.
        KH_SAFE_RELEASE(skin_so_layout);
        KH_SAFE_RELEASE(skin_so_gs);
        KH_SAFE_RELEASE(skin_so_vs);
        KH_SAFE_RELEASE(blend_pf);
        KH_SAFE_RELEASE(samp_pf);
        KH_SAFE_RELEASE(rast_sun);
        KH_SAFE_RELEASE(rast_sun_lo);
        KH_SAFE_RELEASE(rast_sun_pk);   // KH_SUN_PANCAKE twins.
        KH_SAFE_RELEASE(rast_sun_lo_pk);
        KH_SAFE_RELEASE(rast_inject);
        KH_SAFE_RELEASE(vs_sundepth);
        KH_SAFE_RELEASE(layout_sundepth);
        KH_SAFE_RELEASE(vs_sundepth_a);
        KH_SAFE_RELEASE(ps_sundepth_a);
        KH_SAFE_RELEASE(layout_sundepth_a);
        KH_SAFE_RELEASE(vs_sundepth_ao);
        KH_SAFE_RELEASE(layout_sundepth_ao);
        KH_SAFE_RELEASE(sun_instance_vb);
        sun_instance_cap = 0;
        KH_SAFE_RELEASE(vs_seam_inst);   // KH_SEAM_INST.
        KH_SAFE_RELEASE(vs_mirror_inst);
        KH_SAFE_RELEASE(seam_instance_vb);
        seam_instance_cap = 0;
        KH_SAFE_RELEASE(dls_rast);
        for (int khd_i = 0; khd_i < KH_DLS_SLICES_MAX; ++khd_i) KH_SAFE_RELEASE(dls_dsv[khd_i]);
        KH_SAFE_RELEASE(dls_srv);
        KH_SAFE_RELEASE(dls_tex);
        dls_slices = 0;
        dls_size = 0;
        KH_SAFE_RELEASE(locality_srv);
        KH_SAFE_RELEASE(locality_buf);
        locality_cap = 0;
        KH_SAFE_RELEASE(castocc_srv);
        KH_SAFE_RELEASE(castocc_tex);
        KH_SAFE_RELEASE(constant_buffer);
        KH_SAFE_RELEASE(composite_cb);
        for (int khcr_i = 0; khcr_i < 2; ++khcr_i) {
            KH_SAFE_RELEASE(cb_ring[khcr_i]);
            cb_ring_cursor[khcr_i] = 0;
            cb_ring_fresh[khcr_i] = true;
        }
        KH_SAFE_RELEASE(frame_cb);
        KH_SAFE_RELEASE(composite_frame_cb);
        KH_SAFE_RELEASE(dss_test);
        KH_SAFE_RELEASE(dss_test_write);
        KH_SAFE_RELEASE(dss_test_write_mark);   // KH_SSAO_MARK.
        KH_SAFE_RELEASE(dss_mark_clear);
        KH_SAFE_RELEASE(dss_mark_test);
        KH_SAFE_RELEASE(dss_off);
        for (int i = 0; i < 6; ++i) KH_SAFE_RELEASE(blend_modes[i]);
        KH_SAFE_RELEASE(dls_world_blend);
        KH_SAFE_RELEASE(dls_world_fog_blend);
        KH_SAFE_RELEASE(dlsw_srv);
        KH_SAFE_RELEASE(dlsw_rtv);
        KH_SAFE_RELEASE(dlsw_tex);
        KH_SAFE_RELEASE(dlsw_min_blend);
        KH_SAFE_RELEASE(vs_dls_mask);
        KH_SAFE_RELEASE(ps_dls_mask);
        KH_SAFE_RELEASE(ps_dls_mask_a);
        KH_SAFE_RELEASE(ps_ssao_depth);   // KH_SSAO.
        KH_SAFE_RELEASE(ps_ssao_main);
        KH_SAFE_RELEASE(ps_ssao_blur);
        KH_SAFE_RELEASE(ps_ssao_apply);
        ps_ssao_samples = 0;
        KH_SAFE_RELEASE(ssao_dh_srv);
        KH_SAFE_RELEASE(ssao_dh_rtv);
        KH_SAFE_RELEASE(ssao_dh_tex);
        KH_SAFE_RELEASE(ssao_ao_srv);
        KH_SAFE_RELEASE(ssao_ao_rtv);
        KH_SAFE_RELEASE(ssao_ao_tex);
        KH_SAFE_RELEASE(ssao_ao2_srv);
        KH_SAFE_RELEASE(ssao_ao2_rtv);
        KH_SAFE_RELEASE(ssao_ao2_tex);
        ssao_w = ssao_h = 0;
        ssao_hw = ssao_hh = 0;
        KH_SAFE_RELEASE(ssao_cb);
        KH_SAFE_RELEASE(ps_ssao_nzmark);   // KH_NEARZ_MARK.
        KH_SAFE_RELEASE(nzm_srv);
        KH_SAFE_RELEASE(nzm_rtv);
        KH_SAFE_RELEASE(nzm_tex);
        nzm_w = nzm_h = 0;
        release_vmsee_targets();   // KH_VM_SEE.
        vmsee_fail_w = vmsee_fail_h = 0;
        KH_SAFE_RELEASE(ps_vmsee_s0);
        ps_vmsee_samples = 0;
        ps_vmsee_fail = 0;
        KH_SAFE_RELEASE(bs_vmsee_t);
        bs_vmsee_failed = false;
        KH_SAFE_RELEASE(ps_effect_pip);   // KH_PIP_FX.
        ps_effect_pip_tried = false;
        KH_SAFE_RELEASE(pip_scene_srv);
        KH_SAFE_RELEASE(pip_scene_tex);
        KH_SAFE_RELEASE(pip_scene_srv2);
        KH_SAFE_RELEASE(pip_scene_tex2);
        KH_SAFE_RELEASE(pip_scene_rtv[0]);
        KH_SAFE_RELEASE(pip_scene_rtv[1]);
        pip_scene_w = pip_scene_h = 0;
        pip_scene_fmt = DXGI_FORMAT_UNKNOWN;
        KH_SAFE_RELEASE(pip_depth_srv);
        KH_SAFE_RELEASE(pip_depth_tex);
        pip_depth_w = pip_depth_h = 0;
        pip_depth_fmt = DXGI_FORMAT_UNKNOWN;
        dlsw_w = dlsw_h = 0;
        KH_SAFE_RELEASE(blend_ui_replace);
        KH_SAFE_RELEASE(blend_ui_masked);
        KH_SAFE_RELEASE(blend_ui_spill);
        KH_SAFE_RELEASE(blend_alpha_zero);
        KH_SAFE_RELEASE(ps_alpha_zero);
        KH_SAFE_RELEASE(ps_premult_copy);
        KH_SAFE_RELEASE(ps_cov_fix);
        KH_SAFE_RELEASE(ps_inj_depth_a);
        KH_SAFE_RELEASE(ps_maskprime);
        KH_SAFE_RELEASE(ps_maskcast);
        KH_SAFE_RELEASE(ps_rpmerge);   // KH_VOL_REPLAY.
        KH_SAFE_RELEASE(ps_rpmergemir);   // KH_MIR_REPLAY.
        KH_SAFE_RELEASE(ps_dls_world);
        KH_SAFE_RELEASE(ps_dls_world_fog);
        KH_SAFE_RELEASE(ps_seam_foot);

        KH_SAFE_RELEASE(cov_hist_srv);
        KH_SAFE_RELEASE(cov_hist_tex);
        cov_hist_w = 0; cov_hist_h = 0; cov_hist_fmt = 0;
        KH_SAFE_RELEASE(rasterizer);
        KH_SAFE_RELEASE(rasterizer_cull);
        KH_SAFE_RELEASE(rasterizer_front);
        KH_SAFE_RELEASE(vs_white);
        KH_SAFE_RELEASE(ps_white);
        KH_SAFE_RELEASE(layout_white);
        KH_SAFE_RELEASE(white_obj_cb);
        KH_SAFE_RELEASE(white_frame_cb);
        KH_SAFE_RELEASE(rast_white);
        KH_SAFE_RELEASE(dss_white);
        white_tried = false;   // The device died; the next ensure may retry.
        white_ready = false;
        release_bb_capture();
        release_fx_chain();
        release_scene_capture();
        release_mm_capture();   // KH_BLEND_MINMAX.
        release_depth_srv();
        for (KhGlowPyr& khgp_p : glow_pyr) khgp_p.release();   // KH_GLOW_PYR.
        KH_SAFE_RELEASE(ps_glow_seed);
        KH_SAFE_RELEASE(ps_glow_down);
        KH_SAFE_RELEASE(glow_sampler);
        for (KhAnaPyr (&khap_r)[2] : ana_pyr) for (KhAnaPyr& khap_p : khap_r) khap_p.release();   // KH_ANA_PYR.
        KH_SAFE_RELEASE(ps_ana_seed);
        KH_SAFE_RELEASE(ps_ana_down);
        for (int khfs_i = 0; khfs_i < 3; ++khfs_i) {   // KH_FX_SIDE.
            KH_SAFE_RELEASE(fxs_srv[khfs_i]);
            KH_SAFE_RELEASE(fxs_rtv[khfs_i]);
            KH_SAFE_RELEASE(fxs_tex[khfs_i]);
            fxs_w[khfs_i] = fxs_h[khfs_i] = 0;
        }
        for (int khlz_i = 0; khlz_i < 3; ++khlz_i) {   // KH_FX_LINZ.
            KH_SAFE_RELEASE(linz_srv[khlz_i]);
            KH_SAFE_RELEASE(linz_rtv[khlz_i]);
            KH_SAFE_RELEASE(linz_tex[khlz_i]);
            linz_w[khlz_i] = linz_h[khlz_i] = 0;
        }
        // The sun maps, their pyramids and the tier keys' subjects all died
        // above; the flags and the input hash that claim them live outside
        // Resources. render_sun_depth's hash early return is reached from
        // mask_cast_engine with no ensure_resources ahead of it, so a hash
        // left standing here would skip the rebuild for as long as it matched.
        kh_sun_ladder_forget();
        initialized = false;
    }
};

static Resources g_res;
static bool g_reset_hook_installed = false;
inline uint64_t steady_now_ms();   // Defined below; the liveness test needs it here.
inline void reset_session_state();
inline void kh_session_teardown(bool khst_clean);   // KH_SESSION_RESET: both mission-edge teardowns (defined with the reset).

// Forward: the render-thread identity check lives with the reorder state in rendering_integration_injection.hpp.
inline bool reorder_on_render_thread();
// KH_SKIN_POOL - a vertex buffer and the byte offset its vertex array starts
// at. Every buffer but a skin pool chunk holds one vertex array from byte 0 (an
// object's own, or a mesh's, shared by its objects); a chunk (KhSkinPoolChunk)
// holds the skinned poses of many bindings of one
// mesh, each in its slice - so a chunk's pointer alone does not name a
// binding's geometry. Every bind (IASetVertexBuffers' offset), every cache of
// the bound buffer, every run predicate, signature and identity test that can
// meet a skin buffer takes the pair. A pointer compare against a buffer that
// is never a chunk (a cloth's, a chain's, a vertex stage's, a mesh's own)
// stays exact.
struct KhVbRef {
    ID3D11Buffer* vb = nullptr;
    UINT          off = 0u;
    explicit operator bool() const { return vb != nullptr; }
};
struct KhSkinPoolChunk;   // KH_SKIN_POOL (defined below; the pool itself is in rendering_integration_features.hpp).
inline KhClothParams kh_cloth_defaults();   // Defined in rendering_integration_features.hpp.

struct RenderObject {
    int   effect = 0;   // 0 = solid color; >0 = screen-space effect (see EffectId).
    // Effect parameters, effect-specific (set_effect_params); shorter script
    // arrays keep zero/default tails.
    float fx[12] = {};
    const std::string* fx_shader = nullptr;   // Interned resolved user.hlsl path (nullptr =
                                              // builtin; consumed only when effect == KH_EFFECT_CUSTOM)
                                              // - or the resolved .cube path when effect ==
                                              // KH_EFFECT_LUT (same slot, same rule). kh_fx_shader_of.
    // KH_FX_TEX: an effect's own textures, the script's "textures" - an interned set (kh_fx_tex_intern),
    // nullptr = none. Bound for a user effect and (KH_FX_TEX_ALL, kh_fx_tex_on) for any effect pass that has a set;
    // a solid mesh's is stored and never read.
    const KhFxTexSet* fx_tex = nullptr;
    bool  fullscreen = false;   // True = fullscreen triangle (post-processing pass), size unused.
    // Fullscreen passes: true = render post-tonemap over the composited frame
    // (UI included).
    bool  affect_ui = false;
    bool  ui_only = false;   // With affect_ui: mask the pass to the UI phase. Meaningless without affect_ui.
    bool  ui_spill = false;
    bool  localized = false;   // Fullscreen pass masked to a world-space sphere around pos.
    bool  local_inverse = false;   // KH_LOCAL_INVERSE: the mask complemented.
    float local_radius[3] = { 25.0f, 25.0f, 25.0f };   // full-strength radii per SQF axis [x, y, z]
                                                       // (m).
    int   local_shape = 0;   // 0 = sphere/ellipsoid, 1 = cube/mesh mask.
    float local_falloff = 10.0f;   // fade-to-zero band beyond the radius (m).
    bool  banded = false;   // Effect confined to a camera-distance band.
    float band_min = 0.0f;   // Band start (m); fades in over band_falloff before it.
    float band_max = 0.0f;   // Band end (m); <= 0 = unbounded (includes sky).
    float band_falloff = 10.0f;   // Fade width at both band edges (m).
    float pos[3] = {};   // SQF coords [x, y, zASL]; converted at draw.
    // KH_POS_RES (kh_pos_res): engine axes, the exact centre minus pos, valid while pos is pos_res_at.
    float pos_res[3] = {};
    float pos_res_at[3] = {};
    float size[3] = { 1.0f, 1.0f, 1.0f };   // Resolved mesh edge lengths per SQF.
    float size_mul[3] = { 1.0f, 1.0f, 1.0f };
    float rot[3] = {};   // [pitch, yaw, roll] degrees, arma sense (yaw) clockwise from north about
                         // z-up, pitch nose-up about x-east, roll clockwise about y-north; heading
                         // applied outermost). Zero when rot_m came from [vectorDir, vectorUp]
                         // (kh_set_rotation_rows): the script typed axes, not angles.
    float rot_m[9] = { 1.0f, 0.0f, 0.0f,
                       0.0f, 1.0f, 0.0f,
                       0.0f, 0.0f, 1.0f };
    bool  rotated = false;   // False = identity (skip the matrix math).
    // KH_CAST_REFIT: the edge lengths (SQF order, metres) the simulated geometry
    // of a plain cloth reached about the object's centre in its last uploaded
    // buffer, when they exceed size on any axis; 0 = none. Read only through
    // kh_bounds_size_of by bounds tests (culls, caster lists, the sun map fit,
    // the reach lists); never a draw transform (size stays the script's) and
    // never the LOD pick. Written by kh_cloth_upload under g_draw_list_mutex.
    float cast_size[3] = { 0.0f, 0.0f, 0.0f };

    // KH_ATTACH_OFFSET - where the mesh sits INSIDE the frame it follows.
    // Both are inert unless the MATCHING lane is attached: with no attachment
    // there is no frame for an offset to be relative to, so the literal
    // position or rotation the script set stands untouched. The two are
    // independent of each other for the same reason the two lanes are.
    //
    // attach_pos is SQF order [x, y, z] like every other script-facing vector,
    // and is read in the FOLLOWED OBJECT's own axes - x right, y forward, z up
    // at that object's identity. Not world axes, and not the mesh's own
    // rotated frame: an attach_rot never swings the attach_pos.
    // attach_rot is a rotation matrix in ENGINE axes, the same form and the
    // same builder as rot_m (kh_rotation_matrix), composed onto the followed
    // object's basis as attach_rot * basis.
    //
    // They live here and not in KhAttach because they are mesh properties that
    // OUTLIVE an attachment - a script may set one, detach, re-attach and
    // expect it to still hold - and a KhAttach entry is erased the moment both
    // its lanes clear. Being here also means they ride the ordinary staged
    // update-and-commit like every other property instead of being written
    // side-band under a different lock. The price is MEASURED, not estimated,
    // and the measurement is +56 bytes on sizeof(RenderObject), paid on every
    // staging copy the flush and the injection make, plus one bool
    // test per lane per step for the meshes that set neither - which is all of
    // them until a script asks. The DELTA is what was measured and is what the
    // claim rests on; the absolute (296 -> 352 under the offline harness's
    // g++ x86-64) is a different compiler's number and is not asserted for
    // this build.
    float attach_pos[3] = {};
    float attach_rot[9] = { 1.0f, 0.0f, 0.0f,
                            0.0f, 1.0f, 0.0f,
                            0.0f, 0.0f, 1.0f };
    bool  attach_pos_on = false;   // False = skip the compose entirely (the step's fast path).
    bool  attach_rot_on = false;
    // KH_SKEL: the position follows a parent with a skeletal binding ([object,
    // true]). The mesh draws at its authored coordinates relative to the
    // parent's origin, its vertex groups following the parent's memory points
    // through a substituted vertex buffer - KH_SKIN_POOL: a slice of its mesh's
    // skin pool, so it joins an instanced bucket with the other bindings of
    // that chunk (kh_inst_pool_of), each drawing its own pose. Set with the
    // KhAttach entry; cleared when the position is pointed elsewhere.
    //
    // While skel is set the skin owns size (the drawn pose's box, so culling
    // follows the animation) and skel_ctr (that box's centre in the authored
    // frame). kh_skin_upload and the consume-point steps set both with the
    // bytes, under g_draw_list_mutex; a binding or mesh change resets them to
    // the rest box.
    bool  skel = false;
    float skel_ctr[3] = { 0.0f, 0.0f, 0.0f };
    // KH_SKIN_POOL: the vertex base of the skinned pose in its skin pool chunk
    // (the slice's offset / 48; 0 for a chunk's first slice and without one -
    // the routes test it against the slot table's offset, never against 0),
    // set by kh_skin_upload under g_draw_list_mutex with the slot tables it
    // installs, carried to the record's res.w (kh_objrec_fill) for the Sk twins.
    uint32_t skin_vbase = 0u;

    bool  caster_only = false;
    // KH_SHADOW_SWITCH ("castShadow" / "receiveShadow"): cast_shadow false
    // takes the object out of every caster list (kh_shadow_active - casterOnly
    // included); receive_shadow false zeroes every received term in the mesh
    // shaders (the khObjNoRecv lane: lighting0.y per draw, KhObjRec pos.w per
    // instance).
    bool  cast_shadow = true;
    bool  receive_shadow = true;
    // KH_OCC_FORCE ("occluder"): the object's box - exactly its size, at its
    // position and rotation - hides what is behind it in the occlusion cull
    // (KH_VIS_OCC) whether or not anything is drawn there. The script's promise;
    // false leaves the object to the automatic rule (KH_OCC_BOX).
    bool  occluder = false;
    // KH_INFRONT: drawn in the engine's view-model depth slice - the range the
    // first-person weapon and hands occupy, in front of the whole world and
    // depth-sorted against them - by kh_infront_inject, never by the world
    // colour passes. It casts shadows like any mesh while a slice is drawn
    // (g_vm_slice_live; the sun / DLS maps and the mask cast are world-space
    // passes, and only the seam footprint, a world-depth draw, leaves it
    // out), takes no screen-space AO (KH_SSAO runs in the world injection
    // alone), and draws level 0 only (the two slice draws must write
    // identical depth). Effect meshes
    // and fullscreen passes ignore it (kh_in_front). With no view-model slice in the frame
    // (third person) the mesh is not drawn, as the hands are not.
    bool  in_front = false;
    // KH_SHOW_INFRONT ("showInFront"): false hides the mesh from the main view while it is a first-person one (the
    // cycle that last ended drew the view-model slice, g_vm_slice_live); with in_front it is never drawn, exactly as
    // visible false. The main camera's draw and pixel-ownership routes test kh_obj_shown; the cost-side censuses and
    // the routes that must keep a superset (posing, the replay rectangles) test kh_obj_vis; the PIP tests visible
    // (another camera - and an inFront mesh is never in it).
    bool  show_in_front = true;
    bool  two_sided = true;   // addRender3D spawns false (the script-side default).
    // This instance always draws level 0 - kh_lod_pick is skipped at both
    // colour loops, so no crossfade.
    bool  lod_lock = false;
    int   blend_mode = 0;   // 0 normal, 1 additive, 2 multiply, 3 screen, 4 lighten, 5 darken.
    // Every opaque solid (effect 0, depth not Off, not a normal-blend
    // translucent) draws through a bucket: one DrawIndexedInstanced per (mesh,
    // LOD, material pages, blend, depth mode, lit, twoSided, part, route) from
    // the object record buffer (t39) + a 16-byte instance lane. The per-object
    // CB path serves translucents and effect meshes only. Materials: an
    // interned pool-owned set; updates swap the pointer.
    const KhMaterialSet* materials = nullptr;
    // KH_SCENE: this object's slot in the live scene, assigned by
    // add_render_object and carried through every copy the script side makes.
    uint32_t slot = 0xFFFFFFFFu;
    int   mesh = 0;   // Mesh registry id 0 = "box"; published ids only - see mesh_id_clamp.
    float color[4] = { 1, 1, 1, 1 };
    bool  lit = false;
    float light_ambient = 0.40f;   // base-color fraction kept in full shadow.
    float light_diffuse = 0.60f;   // n.L-scaled fraction (ambient + diffuse ~ 1).
    uint64_t seq = 0;   // Creation order (fullscreen pass chaining).
    // Transient, set only on the flush's staging copies, never stored. An
    // injected object's part is drawn by the injection; the flush stages a part
    // only for objects it owns.
    uint8_t draw_part = 0;
    DepthMode mode = DepthMode::TestOnly;
    bool  visible = true;
    bool  timed = false;
    float birth_time = 0.0f;
    double fx_t0 = 0.0;   // Creation stamp of the session clock in double (kh_fx_time's origin); never re-armed.
    float fade_in = 0.0f;
    float hold_dur = 0.0f;
    float fade_out = 0.0f;

    // KH_CLOTH. The two roles are independent and a mesh may hold neither,
    // either or both - a collider that is itself cloth is legal and simply
    // collides against its own rest shape, which is what a script asking for
    // it almost certainly meant. The parameters live here, not in the instance
    // table, for the same reason attach_pos does: they are mesh properties
    // that OUTLIVE the simulation, so turning cloth off and on again keeps the
    // tuning. The instance table holds the STATE, which does not.
    bool cloth_sim = false;         // "clothSimulation": this mesh simulates.
    bool physics_collider = false;    // "physicsCollider": this mesh pushes cloth around.
    KhClothParams cloth = kh_cloth_defaults();
    float physics_col_margin = 0.0f;      // Collider surface offset (m), on top of the cloth's thickness.
    float physics_col_friction = 1.0f;    // Collider's half of the friction product.
    bool  physics_col_inside = false;     // Contain cloth rather than exclude it.
    bool  physics_col_convex = false;     // KH_COL_CONVEX: collide as the mesh's convex pieces' planes.
    // KH_CHAIN: "chainSimulation" - the mesh's skeleton simulates as a chain
    // of rigid bodies (KhChainState). Only the switch is here, for the passes
    // that must treat the object's buffer as its own (instancing); the
    // settings hold game objects and live game-side (KhChainCfg). Exclusive
    // with cloth_sim. While it is on, a plain object's size is the chain's
    // (kh_chain_rest_size, kh_cloth_upload) and the script's is kept in
    // size_mul, as under a skeletal binding.
    bool  chain_sim = false;
    // KH_SIM_LOD: "simulationLod" - the cloth / chain solver's level of detail
    // (kh_simlod_share): on/off, and floor (the least share of iterations /
    // substeps, 0.05..1), full distance and min distance (m), full and min
    // screen share (of the screen height). On by default at these values,
    // fitted to a 1 x 2 m cape (sphere ~1.1 m): whole to 25 m, the floor
    // from 100 m, and the shares are what that cape spans there at the
    // default field of view (m11 ~1.33) - 0.06 and 0.015.
    bool  sim_lod_on = true;
    float sim_lod[5] = { 0.25f, 25.0f, 100.0f, 0.06f, 0.015f };
};

// Effect ids shared by meshes (localized, clipped to the mesh's screen
// footprint) and fullscreen passes. 10 and 11 sample the engine depth buffer
// per pixel.
enum class EffectId : int {
    Solid = 0, Invert = 1, ColorGrade = 2, Vignette = 3, Chromatic = 4,
    Grain = 5, Sharpen = 6, Blur = 7, Bloom = 8, Distortion = 9,
    Outline = 10, Pulse = 11, Halation = 12, Fog = 13,
    LensFlare = 14, Anamorphic = 15, SunFlare = 16, Glitch = 17,
    Clarity = 18, Deband = 19, RainLens = 20,
    // CRT tube simulation.
    Crt = 21,
    Ssgi = 22,
    Fogscatter = 23,
    // 24 - 30 are the chain's internal side ids (SSGI's 24 - 26, with 27 - its pyramid's level draw - retired by
    // KH_SSGI_SEED; KH_FX_SIDE's 28 - 30), refused to a script (effect_id_from_gv).
    DynLightFog = 31,   // KH_DLF.
};
// User effect sentinel: RenderObject.effect for a custom ".hlsl" pass
// (fx_shader carries the resolved path).
static constexpr int KH_EFFECT_CUSTOM = 100;
// LUT sentinel: a ".cube" 3D-LUT grade; fx_shader carries the resolved path.
// The lattice binds at t19 and the builtin uber shader applies it, so
// KH_EFFECT_CUSTOM resolve sites never see this id.
static constexpr int KH_EFFECT_LUT = 101;

// Rain state: written only inside the lock-held flush, read only at CB fill
// sites inside serialized windows - no atomics needed.
static float    g_rain_vel[3] = {};
// Integrated, never multiplied: accumulates dt * factor per flush and rides
// fxParams1.w; the shader multiplies only by the user's speed param.
static double   g_rain_phase = 0.0;   // Integrated in double: a float accumulator loses its dt to rounding after hours.

static std::unordered_map<std::string, RenderObject> g_draw_list;
static std::mutex g_draw_list_mutex;

// Every object is trivially copyable so the live-scene sync is a memcpy per
// dirty object; the string / material members are interned pointers.
static_assert(std::is_trivially_copyable<RenderObject>::value, "RenderObject must stay a POD snapshot (KH_SCENE)");
static constexpr float    KH_SCENE_CELL_M = 64.0f;   // Grid cell edge, engine x / z.
// A slot with no cell (a fullscreen pass). Not KH_SCENE_NONE: widened to a key,
// 0xFFFFFFFF is the cell x 0, z -1 (SQF x in [0, 64), y in [-64, 0)). x = INT32_MIN
// is no cell: kh_scene_cell_key keeps every index within 2^30.
static constexpr uint64_t KH_SCENE_CELL_NONE = 0x8000000000000000ull;

struct KhSceneCell {
    std::vector<uint32_t> slots;
    float ymin, ymax, rmax;   // Loose bounds (grow only; reset when the cell empties).
};

struct KhScene {
    std::vector<RenderObject> objs;   // By slot.
    std::vector<uint8_t>      alive;
    std::vector<uint64_t>     cell;   // Grid key per slot (KH_SCENE_CELL_NONE for none).
    std::unordered_map<uint64_t, KhSceneCell> grid;   // Mesh objects only, by centre cell.
    uint32_t alive_n = 0;
    uint64_t gen = 0;   // Bumps on every sync that changed anything.
};
static KhScene g_scene;
// KH_LOD_BOUNDS: the lowest mesh id an object was gridded under while that id was
// unpublished - its cell radius read mesh 0's bounds (the id clamps); 0xFFFFFFFF =
// none. With g_scene: kh_scene_grid_insert lowers it, flush_locked's housekeeping
// grids those objects again once the id reads published.
static uint32_t g_scene_unpub_min = 0xFFFFFFFFu;
// Staging side (under g_draw_list_mutex).
static std::vector<RenderObject*> g_scene_stage;   // Slot -> map value (nullptr = removed).
static std::vector<std::string>   g_scene_handle;   // Slot -> handle (the flush's expiry erase).
static std::atomic<bool> g_shadows_off{ false };   // Game thread writes; every thread reads.
#define KH_ATTACH_OFF_NONE  0xFFFFFFFFu
struct KhSkinInst;
struct KhSkinRest;   // KH_SKIN_CULL: KhAttach::cull_rest (defined with the skin in rendering_integration_features.hpp).
struct KhSkinXf { float q[9]; float t[3]; };   // A proxy against its parent: rows Q, position t (KH_SKEL; defined here for KhAttach's scratch).
struct KhAttach;
// KH_ATTACH_GT_SNAP - ONE game-thread sample of a binding: everything it follows, read TOGETHER from the object's
// chosen copy at Draw3D, which is the instant the engine's own drawn state is coherent (measured: a helper attached
// to a bone reads there exactly where the engine has it for the frame being drawn, standing and moving alike - the
// sphere-and-dot test). That was measured while helpers were still drawn; KH_PXY_HIDE changed whether a helper is
// SUBMITTED, not when the engine refreshes it - it is refreshed beside its parent either way (KH_PXY_HIDE's numbers).
// The render thread used to read the helpers itself at its first step of a cycle, and the engine refreshes an
// attached child DURING the frame, after that step: 54 % of those reads changed again before the injection (up to
// 76 mm), which a parent-relative placement cancels while the unit travels and shows in full when a bone moves
// against a standing parent - the in-place animation lag.
//   par  = the object the binding is placed RELATIVE to: the parent of a skeletal or memory-point binding, the
//          followed object itself for a plain one (and for a binding that follows a rotation alone, that object -
//          kh_attach_parent_gv). 12 floats: position (SQF order), then the basis rows.
//   px   = the helpers, the same 12 floats each: a skeletal binding's proxies in order, or a memory-point
//          binding's one proxy (a refused one carried from the previous sample - KH_GTS_HOLD). Empty for a plain
//          binding.
//   rot  = the ROTATION lane's own object, when it is neither par nor the one helper (a script's own object);
//          rot_of names that object (its base, KH_ATTACH_ROT_OF): the step takes rot only while the rotation lane
//          still follows it, so a lane re-pointed since the sample never turns with the old object.
// gen / xb name the binding the sample was taken for (a skeletal binding's skel_gen; the position lane's object, or
// the rotation lane's where there is no position lane - kh_attach_key_gv),
// so a re-bind cannot be served a sample of what it followed before. seq / qpc order the ring and place it against
// the cycle's depth clear. Written by the game thread and read by the render thread's step, both under
// g_draw_list_mutex.
static constexpr uint32_t KH_GTS_N = 4u;   // Samples kept: the drawn frame is at most one or two behind the newest.
struct KhGtSnap {
    bool      ok = false;
    bool      rot_ok = false;
    uint64_t  seq = 0;
    int64_t   qpc = 0;
    uint32_t  gen = 0;
    uintptr_t xb = 0;
    float     par[12] = {};
    float     rot[12] = {};
    uintptr_t rot_of = 0;   // KH_ATTACH_ROT_OF: the base of the object rot was read for (0: none).
    std::vector<float> px;
    std::vector<float> pxd;   // KH_PXY_SMOOTH: per helper 3 floats (SQF order), estimator output minus px's reading.
};
struct KhAttach {
    // KH_ATTACH_GT_SNAP: the ring, and the page caches and copy choices of the reads that fill it (the Draw3D
    // sampler's, game thread).
    KhGtSnap  gts[KH_GTS_N];
    uint32_t  gts_w = 0;
    uint64_t  gts_cyc = ~0ull, gts_pick_seq = 0;   // The cycle last picked for, and its sample (kh_gts_for_cycle).
    uint32_t  gts_gen = 0;
    uintptr_t gts_xb = 0, gts_pb = 0, gts_rb = 0;
    uint32_t  gts_par_off = KH_ATTACH_OFF_NONE, gts_rot_off = KH_ATTACH_OFF_NONE;
    uintptr_t gts_par_vb = 0, gts_par_bb = 0, gts_rot_vb = 0, gts_rot_bb = 0;
    uintptr_t gts_par_avb = 0, gts_par_abb = 0, gts_rot_avb = 0, gts_rot_abb = 0;   // KH_ATTACH_VS_CACHE.
    std::vector<uint32_t>  gts_off;
    std::vector<uintptr_t> gts_vb, gts_bb, gts_avb, gts_abb;
    // KH_RT_PAINT_READ: the render thread's own read of what the sampler last read (rt_par: the parent or its helper;
    // rt_rot: the rotation object, nil when the lane has none of its own), with its own page caches, for the cast
    // fire's paint and the final step's mesh when this frame's own sample has not landed. rt_snap is that read,
    // rt_cyc the cycle it is for.
    game_value rt_par, rt_rot;
    uint32_t  rt_par_off = KH_ATTACH_OFF_NONE, rt_rot_off = KH_ATTACH_OFF_NONE;
    uintptr_t rt_par_vb = 0, rt_par_bb = 0, rt_rot_vb = 0, rt_rot_bb = 0, rt_pb = 0, rt_rb = 0, rt_xb = 0;
    uintptr_t rt_par_avb = 0, rt_par_abb = 0, rt_rot_avb = 0, rt_rot_abb = 0;   // KH_ATTACH_VS_CACHE.
    uint32_t  rt_gen = 0;
    std::vector<uint32_t>  rt_off;
    std::vector<uintptr_t> rt_vb, rt_bb, rt_avb, rt_abb;
    KhGtSnap  rt_snap;
    uint64_t  rt_cyc = ~0ull;
    // KH_ATTACH_ROT_OF: what the sampler named rt_par / rt_rot for - the binding's gen and key (kh_attach_key_gv's
    // base) and the rotation lane's object (its base; 0 while rt_rot is nil). The paint read reads a binding only
    // while it is still that one (one re-bound since - another key object, or a new skel_gen - reads nothing until
    // the next Draw3D names its objects), and rt_rot only while the rotation lane still follows what it was named
    // for.
    uint32_t  rt_for_gen = 0;
    uintptr_t rt_for_xb = 0, rt_rot_for = 0;
    // KH_RT_PAINT_READ: the read the final step drew for the mesh (the frame's own sample not landed), kept for its
    // check against that sample at the next cycle's final step (kh_gts_for_cycle).
    KhGtSnap  rt_chk;
    uint64_t  rt_chk_cyc = ~0ull;
    bool      rt_crew = false;   // rt_snap placed by the crew prediction (KH_CREW_LAG): the repick's alone.
    // KH_ATTACH_MAN_HELPER: a character's own visual state is not written while it rides in a vehicle (the engine
    // draws the crew from the vehicle, and the state keeps the pose it had when the character got in), but a helper
    // attached to it is: attachTo places it from the character's drawn transform. A lane naming a Man therefore gets
    // a pooled helper attached at the character's origin (kh_pxy_pool_get, memory point ""), and the Draw3D sampler
    // reads the helper wherever it would have read the character (kh_attach_org_read) - a skeletal binding's parent
    // included, whose frozen state the bones were made relative to (cancelled until a bone lay more than
    // KH_SKEL_REACH_M from it, and never under rotation false). One helper per character, shared through the pool by
    // every lane and binding on it and deleted with the last (kh_attach_proxy_reap). org_*_for is the object the
    // helper stands in for (kh_attach_org_sync retires one that no longer does); org_*_seq is the sample that first
    // saw the helper after it was made (the pool's made_seq + 1; 0: not known, the first sample of the lane), which
    // the sampler skips, because attachTo places a new helper at the next simulation step - one made frames ago is
    // read from the lane's first sample.
    // Game thread, under g_draw_list_mutex; the render thread never reads them.
    game_value  org_pos, org_rot;
    const void* org_pos_for = nullptr;
    const void* org_rot_for = nullptr;
    uint64_t    org_pos_seq = 0, org_rot_seq = 0;
    game_value obj_pos;   // Nil = position not attached.
    game_value obj_rot;   // Nil = rotation not attached.
    // KH_ATTACH_BONE. proxy is a LOCAL simple object (KH_ATTACH_BONE_SHAPE)
    // that the engine keeps attached to a memory point of bone_parent, so the
    // two lanes above can read a bone's transform through the ordinary raw
    // path with no new engine assumption: obj_pos (and obj_rot, when the
    // script asked to follow the bone's rotation) hold THIS proxy, not the
    // parent. Nil = no bone attachment and the lanes mean what they always
    // did.
    //
    // bone_parent is held for the death test, and it is the parent the Draw3D
    // sampler reads beside the proxy (kh_attach_parent_gv). attachTo does not
    // destroy the proxy when its parent dies, so a mesh following a bone of a
    // deleted vehicle would otherwise hang at the parent's last position
    // forever - the proxy is still perfectly alive. Both are tested in
    // kh_attach_step and either one being empty kills the mesh.
    //
    // The proxy is OURS (pooled: shared by every binding on the same parent and
    // memory point), so it must be deleted when its last binding ends or it is
    // a leaked game object. Attachments end under g_draw_list_mutex (and, in a
    // park, under the graphics lock), where an SQF call may not be made, so the
    // deletion is queued; see g_attach_proxy_dead.
    game_value proxy;
    game_value bone_parent;
    // KH_SKEL - a skeletal binding ([object, true]). skel_proxy holds one
    // KH_ATTACH_BONE_SHAPE per memory point of bone_parent that a bone of the
    // mesh names AND the skin drives something through (kh_skel_plan,
    // KH_SKEL_PROXY_CULL), attached
    // follow-bone like the single proxy above and OURS in the same way, and
    // skel_mem their lower-cased names in the same order. The two lanes follow
    // the PARENT here, not a proxy: obj_pos is bone_parent, and obj_rot is
    // bone_parent while the script's rotation boolean is true. The proxies are
    // read beside the parent by the Draw3D sampler (KH_ATTACH_GT_SNAP), and the
    // render thread's kh_attach_step stashes that pair for kh_skin_sync
    // (KH_SKEL_SAMPLE).
    // skel_gen is a process-wide serial taken at every binding, so the skin's
    // per-proxy read caches never outlive the proxies they were for.
    bool skel = false;
    std::vector<game_value> skel_proxy;
    std::vector<std::string> skel_mem;
    uint32_t skel_gen = 0;
    // KH_SKEL_SAMPLE - the proxies and the parent they were read beside,
    // stashed by kh_attach_step from the cycle's Draw3D sample
    // (KH_ATTACH_GT_SNAP), raw - before the attach offsets. kh_skin_sync builds
    // its poses from the stash while it is fresh (KH_SKEL_SAMPLE_KEEP_MS); a
    // stale one gives it no new pose. Per proxy 12 floats: position (SQF
    // order) then the basis rows; the parent the same 12. Written under
    // g_draw_list_mutex by the step, read under it by the sync.
    std::vector<float>     skel_smp;
    float                  skel_smp_par[12] = {};
    std::vector<float>     skel_smp_d;   // KH_PXY_SMOOTH: per proxy 3 floats, the sample's pxd (zero: none).
    bool                   skel_smp_ok = false;
    uint32_t               skel_smp_gen = 0;    // The skel_gen the stash was taken for.
    uint64_t               skel_smp_ms = 0;     // Its steady stamp (KH_SKEL_SAMPLE_KEEP_MS).
    // KH_SKEL_DRAW: the cycle (g_topo_cycles) whose render-thread step took
    // this stash and decided its skin - usually the cast fire's; stamped when
    // the maps fail or the pool's chunk throws too (the last pose stands),
    // and not by a step that culled it, nor when the chunk throws for a
    // binding whose buffer stands culled (KH_SKIN_CULL: every later step of the
    // cycle decides again). Later steps of that
    // cycle keep both unless a later pick takes another sample (skel_smp_seq,
    // the sample stashed; skel_draw_seq, the sample skinned) - the frame's own
    // landing, or a render-thread read of a changed pose (KH_RT_READ_SER) -
    // when it stashes and skins again, so the injection and everything after
    // it draw one pose, the one the rigid bindings draw.
    uint64_t               skel_smp_cycle = ~0ull;
    uint64_t               skel_smp_seq = 0;
    uint64_t               skel_draw_seq = 0;
    // KH_ATTACH_CYCLE: the cycle (g_topo_cycles) in which the render thread's
    // step last wrote this entry's lanes into the mesh - the consume point's
    // sample, the one that draws. A park's step (kh_park_run) leaves an entry
    // the render thread sampled this cycle alone, so a park's late draw lands
    // where the injection drew; an entry the render thread has not reached
    // this cycle takes the park's step.
    uint64_t               smp_cycle = ~0ull;
    // KH_SKEL_DRAW - the skinning AT THE CONSUME POINT. The worker pipeline
    // (kh_skin_sync at Draw3D, a pool job, kh_skin_upload in the flush) hands
    // the drawers a buffer skinned from an older sample, so a skeletal mesh
    // drew a frame behind its body. So the render-thread step skins the stash
    // it just took into the same buffer the upload installs, and draws that
    // (kh_skin_draw_prep / _verts / _finish). skin is the instance
    // kh_skin_sync keeps for this handle (its drive tables and rest
    // snapshot; set under g_draw_list_mutex); the draw_* vectors are this
    // step's scratch, under g_draw_list_mutex and render thread only, except
    // draw_out / draw_met, which kh_skin_draw_verts fills on whichever pool
    // thread takes the binding's chunk (KH_SKIN_FORK) while the render thread
    // holds the mutex and waits for it.
    std::shared_ptr<KhSkinInst> skin;
    std::vector<KhSkinXf>   draw_pose;
    std::vector<uint8_t>    draw_ok;
    std::vector<float>      draw_a;
    std::vector<MeshVertex> draw_out;
    std::vector<float>      draw_met;
    // KH_SKEL_SHAPE: the shape key of the pose the step last wrote and the
    // metric positions that took it. Render thread, under g_draw_list_mutex.
    std::vector<float>      draw_shape_ref;
    uint32_t                draw_shape_key = 0u;
    bool                    draw_shape_gpu = false;   // KH_SKIN_GPU: draw_shape_ref holds a palette (kh_skin_so_shape_note), not positions.
    // KH_SKIN_CULL: the buffer holds an older pose than the box the binding
    // carries (a step culled it: no pass could draw it); the next step that
    // skins it, or a catch-up (kh_skin_cull_catchup), clears it. And the last
    // bound the cull took, by its inputs (the rest snapshot and the maps): the
    // bound is a pure function of them, so equal inputs reuse it.
    bool                    skin_culled = false;
    std::shared_ptr<const KhSkinRest> cull_rest;
    std::vector<float>      cull_a;
    float                   cull_bc[3] = { 0.0f, 0.0f, 0.0f };
    float                   cull_bs[3] = { 0.0f, 0.0f, 0.0f };
};
static constexpr uint64_t KH_SKEL_SAMPLE_KEEP_MS = 250;   // A stash older than this gives the sync no new pose.
// The render cycle counter (bumped at the main depth clear; the frame
// topology census's, defined here because the stash below stamps on it).
static uint64_t  g_topo_cycles = 0;
// Published copy for game-thread readers (kh_park_run's step, kh_skin_sync,
// kh_fx_late_gt and its chain-only flush, flush_frame's park test). Stored and
// zeroed with g_topo_cycles.
static std::atomic<uint64_t> g_topo_cycles_pub{ 0 };
inline uint64_t steady_now_ms();   // Defined below; the stash's freshness needs it.
// Deliberately never destructed, the idiom the shader pool already uses: this
// static owns SQF references (as g_pxy_pool, g_crew and g_chain_end, kept the
// same way, do, and the delete queue g_attach_proxy_dead, drained every
// Draw3D), and running a
// game_value's release at static teardown would touch the engine's allocator
// after the engine is done with it. Both mission edges empty it
// (reset_retained_state), so it is normally empty long before then anyway.
static std::unordered_map<std::string, KhAttach>& g_attach =
    *(new std::unordered_map<std::string, KhAttach>());   // Under g_draw_list_mutex.
// The step's gate, so a session with no attachment pays one relaxed load per
// frame and takes no lock. Stored from the size under the mutex.
static std::atomic<uint32_t> g_attach_n{ 0 };
inline bool kh_stats_on();   // Defined with KH_PROF at the top of the namespace.
// KH_SKIN_FORK - one binding whose vertices this step skins across the pool:
// its loop tail (the upload, the box, the draw centre, the scene mark) waits
// for the vertices. Everything else of the binding - its stamps, its cycle
// sample - stays where it was in the loop.
struct KhSkinDrawJob {
    const std::string* h;
    KhAttach*          a;
    RenderObject*      o;
    double             root[3];   // KH_POS_RES: the skeletal root, exact.
    float              bc[3], bs[3];
    bool               moved;
    bool               ok;
    bool               rt;   // reorder_on_render_thread() at the binding (the restep's gate).
    // KH_SKIN_GPU: the buffer kh_skin_so_batch skinned this binding into on the
    // GPU (bc / bs then hold the pose's bound); null = the pool skins it.
    // KH_SKIN_POOL: the pair - a chunk's slice, or the binding's own at 0.
    KhVbRef            so_vb;
    // KH_SKIN_CULL: culled (so_vb is the binding's buffer, bc / bs the bound,
    // nothing is drawn into it); bnd = bc / bs already hold the bound
    // (kh_skin_cull_pass), which the batch then takes; the stamps the step
    // overwrote, handed back to a culled binding.
    bool               culled;
    bool               bnd;
    uint64_t           prev_smp_cycle;
    uint64_t           prev_draw_seq;
};

struct RenderStats {
    uint64_t flushes = 0;   // Flush attempts with work queued.
    uint64_t lock_retries = 0;   // Individual failed lock acquisitions.
    uint64_t lock_failed_frames = 0;   // Frames dropped after all retries failed.
    uint64_t ui_flushes = 0;   // UI-phase flush attempts with work queued.
    uint64_t composite_injections = 0;   // pre-translucent injection events (once per scene frame).
    uint64_t pip_injections = 0;   // KH_PIP: injections into PIP passes.
    uint64_t pip_meshes = 0;       // KH_PIP: draws they made.
    uint64_t pip_fx = 0;           // KH_PIP_FX: localized fullscreen passes drawn into PIP passes.
    // KH_CLOTH. Every one of these answers a question that cannot be answered
    // by looking at the screen: whether the sim ran at all, whether it is
    // keeping up, and whether it is rescuing itself behind your back.
    // KH_CLOTH / KH_CHAIN, published by kh_cloth_upload: instances simulating
    // right now, the frames one was still busy from the previous one (since the
    // stats were armed - a steady rise means the workers cannot keep up), the collider
    // meshes gathered last frame and the live physics affectors.
    uint64_t cloth_instances = 0;
    uint64_t cloth_skipped = 0;
    uint64_t physics_colliders = 0;
    uint64_t physics_affectors = 0;
    // KH_SKEL, published by the upload like the cloth's: skeletal meshes with a
    // vertex buffer, the bones of their skeletons, the bones among those whose
    // own name matched a memory point (0 with skinBones above 0 means no name
    // matched), and the memory-point proxies following the parents.
    uint64_t skin_meshes = 0;
    // KH_SKIN_POOL: of those, the ones whose pose lives in a skin pool chunk
    // (published by the upload); and, since the stats were armed, the skeletal
    // instances the colour buckets drew from a chunk (one per instance per
    // draw - a crossfading one twice).
    uint64_t skin_pooled = 0;
    uint64_t skin_inst = 0;
    uint64_t skin_bones = 0;
    uint64_t skin_bones_driven = 0;
    uint64_t skin_proxies = 0;
    // Changed poses that waited a frame because that mesh's last job was still
    // running on the pool - the skin's clothSkipped. Since the stats were armed.
    uint64_t skin_skipped = 0;
    // Full CPU skins by the skin pool's workers (kh_skin_run with vertices: a
    // binding's first result, an unhide, a device reset, a span no render-thread
    // step covers) - not in skinCpu, the steps' count. Since the stats were armed.
    uint64_t skin_pool_full = 0;
    // KH_SKIN_GPU_MOVE, since the stats were armed: bindings moved from a
    // CPU-skinned buffer to GPU skinning (kh_skin_gpu_move), and the device's
    // refusals of a GPU-skinning buffer (the pool slice and one of the
    // binding's own, at a first install or a move) or of a mesh's rest stream
    // - each leaves a binding on the CPU path until the retry - or of memory
    // for the stream-output objects (KH_SKIN_SO_RETRY: every binding, likewise).
    uint64_t skin_gpu_moves = 0;
    uint64_t skin_gpu_refused = 0;
    uint64_t chain_instances = 0;
    uint64_t chain_skipped = 0;
    // KH_SIM_LOD, published with the cloth's: the solver work (substeps x
    // iterations over the instances stepped last frame) at the counts the
    // LOD chose and at the objects' own dials; their ratio is the share run.
    uint64_t simlod_work = 0;
    uint64_t simlod_full = 0;
    uint64_t composite_meshes = 0;   // Meshes drawn through the composited path.
    // KH_OCC_STATS, since the stats were armed: the occluder boxes the injection's
    // occlusion cull drew into its grid (kh_vis_occ_build; a box that covered no
    // cell is not counted), and the objects inside the view it then hid (what
    // the frustum rejects is not counted).
    uint64_t vis_occluders = 0;
    uint64_t vis_occluded = 0;
    uint64_t textured_draws = 0;   // per-submesh textured draws issued (KH_TEXTURED).
    uint64_t meshes_released = 0;   // KH_MESH_FREE: registry meshes released by the idle GC.
    uint64_t textures_released = 0;   // KH_TEX_GC: cached texture views released idle.
    // FBX files parsed + registered (misses of both the id map and the binary
    // mesh cache).
    uint64_t fbx_imports = 0;
    // KH_DL_DIAG - the dynamic-light pool's upkeep. Harvests taken by the
    // injection and by the render-thread flush's fallback (a cycle with no
    // accepted injection); harvests the sampler knew were incomplete (the TTL
    // sweep ran instead of presence); author windows refused at the per-span
    // cap; lit mesh fills that found the pool stale (older than
    // KH_DL_POOL_STALE_MS: those draws got no dynamic light); light-ring
    // appends that failed; and the most lights one draw received.
    uint64_t dl_harvests_inject = 0;
    uint64_t dl_harvests_flush = 0;
    uint64_t dl_harvests_incomplete = 0;
    uint64_t dl_windows_refused = 0;
    uint64_t dl_stale_fills = 0;
    uint64_t dl_ring_fails = 0;
    uint64_t dl_mesh_lights_max = 0;
    // KH_DL_DIAG (CN-4) - what can change the dynamic lights for one frame. Pool lights a complete harvest's presence
    // sweep dropped with no sighting after the harvest that created them (a light drawn in one span only, a copy placed
    // at a wrong origin, the placements of a light moving past the match cube that no later upload re-sighted).
    // Harvests whose brightest candidate - the main-pass reference's evidence (ref_hist, KH_DL_REF_MEDIAN: a placed
    // window or an upload re-sighted through its buffer's origin) - was brighter / dimmer than the last such
    // harvest's (kh_dl_ref_brighter), and whose previous such was a move this one took back (a one-harvest excursion
    // - the median keeps it from the reference). New pool entries
    // repeating an older entry's record a nonzero whole number of land-grid cells away at its height (kh_dl_copy_of:
    // another origin's placement of that light, or a lamp of the same model grid-aligned with it). And
    // dynamicLightFog lights, per pass and frame, whose light-map slot is not published (unshadowed there).
    uint64_t dl_blinks = 0;
    uint64_t dl_ref_up = 0;
    uint64_t dl_ref_down = 0;
    uint64_t dl_ref_spike_up = 0;
    uint64_t dl_ref_spike_down = 0;
    uint64_t dl_copies = 0;
    uint64_t dlf_unshadowed = 0;
    // KH_DL_PRES_MEMO - the light lists presence matched against the pool (uploads past the count check), and how
    // many of those replayed a remembered match instead of running it.
    uint64_t dl_pres_calls = 0;
    uint64_t dl_pres_memo_hits = 0;
};
static RenderStats g_stats;
// KH_STATS_ARMED: collection is off until the first getRenderStats; a
// resetRenderStats zeroes the counters and disarms it again. Every counter
// write is gated on kh_stats_on (kh_stat / kh_stat_add, or a test at the site)
// so a disarmed session pays one relaxed load per site and accumulates nothing.
// g_stats_armed / kh_stats_on: defined with KH_PROF at the top of the file.
inline void kh_stat(uint64_t& khst_c) { if (kh_stats_on()) ++khst_c; }
inline void kh_stat_add(uint64_t& khst_c, uint64_t khst_n) { if (kh_stats_on()) khst_c += khst_n; }
// The live flush count the graves and the band dedupe key on (a mechanism,
// reset only by the session teardown, which empties every grave and latch that
// holds one first). Advanced by flush_frame on the game thread and read by the
// render thread's flush, so atomic; a reader one step behind holds a grave one
// flush longer.
static std::atomic<uint64_t> g_flush_serial{ 0 };

// sampleDesc.Count of the adopted main depth. KH_XTHREAD_ATOMIC: written by flush_locked (render thread or a
// park), read on the game thread (kh_fsaa_world_standdown, getRenderStats).
static std::atomic<uint32_t> g_scene_depth_samples{ 0 };
// INTENTIONAL: at FSAA 1x (single-sample main depth) every world mesh stands
// down - flush and injection - while fullscreen passes keep running. The mesh
// path is validated against the multisampled scene only. At 1x the game thread
// clears the main depth, so the clear hook names it the render thread, and the
// engine's render thread - which draws the 1x scene - is outside every hook's
// render-thread gate: the injection, PIP, in-front and seam routes, keyed on its
// scene passes, never run (measured, probe FS: no injection or PIP injection
// at 1x). It never flushes either, so the watchdog parks every 1x frame, and
// that flush draws no world mesh into the frame (the white placeholder
// included; its sun and dynamic-light maps can still render, unread). A runtime
// FSAA change returns to the bootstrap (KH_WATCH_STALL, KH_FSAA_SWITCH).
inline bool kh_fsaa_world_standdown() {
    return g_scene_depth_samples.load(std::memory_order_relaxed) == 1;
}

static const char* const KH_SHADERS_COMPILING = "shaders compiling";
static UINT g_main_depth_w = 0;
static UINT g_main_depth_h = 0;

// Depth values were written through the scene viewport depth range; UI-phase
// shaders must linearize with the same range. Updated by the scene flush;
// initialized to the empirically probed defaults.
static float g_scene_vp_min_d = 0.011f;
static float g_scene_vp_max_d = 0.999f;

// Shaders convention: row_major, clip = p * viewProj.

// HLSL sources are concatenated per unit; no #include, since the cache key
// hashes the assembled source. Rules: fxc has no linker pass, so a function
// body must precede its caller in the unit; cb.hlsl is the shared prefix (a
// helper both chains need goes there); twins move together (flush / injection
// mesh loops, PSMain / PSComposite / PSEffect, the HLSL and C++ sides of the
// CB, the two sun-verdict chains); any .hlsl edit changes that unit's cache
// key, so batch shader edits.
enum KhHlslRes : int {
    KH_HLSL_CB = 0,          // The shared prefix (cb.hlsl).
    KH_HLSL_WHITE,
    KH_HLSL_STATIC,          // Static entry points (no depth access).
    KH_HLSL_COMPOSITE,       // Injected path, first half.
    KH_HLSL_COMPOSITE2,      // Injected path, second half (joined at the compile site).
    KH_HLSL_EFFECT,          // Effect uber-shader (t0 scene, t1 depth), three parts.
    KH_HLSL_EFFECT2,
    KH_HLSL_EFFECT3,
    KH_HLSL_DEPTH_RESOLVE,
    KH_HLSL_SUNPF,
    KH_HLSL_VMIR_CS,         // Standalone b2 patch (a draw since KH_MIR_DRAW; the name stays).
    KH_HLSL_SSAO,            // KH_SSAO: the AO passes and the near-plane marker (standalone).
    KH_HLSL_SKIN,            // KH_SKIN_GPU: the stream-output skinning vertex shader (standalone).
    KH_HLSL_N
};

struct alignas(16) ConstantData {
    // per-object block (b0).
    float center_size[4];
    float center_rel[4];
    float color[4];
    float fx0[4];   // Effect parameters [0..3]; on the mesh fill sites xyz = the camera.
    float fx1[4];
    float fx_meta[4];   // Effect id, time (s), screen width, screen height.
    float depth_params[4];
    float size_axes[4];   // Xyz = mesh edge lengths (engine axes), w = blend mode id.
    float local0[4];   // Xyz = mask center (engine space), w = shape (0 sphere, 1 cube).
    float local1[4];   // x = falloff (normalized to mean radius), y = localized flag.
    float local_radii[4];   // Xyz = mask radii (engine axes).
    float band0[4];   // x = band min (m), y = band max (m, <=0 unbounded), z = falloff (m), w =
                      // Banded flag.
    float lighting0[4];   // x = lit flag, y = 1 receiveShadow off (KH_SHADOW_SWITCH), z = ambient, w = diffuse.
    // x = 1 on an effect-chain pass whose near-plane marker (t37) is this cycle's (KH_NEARZ_MARK), else 0; y =
    // object view-distance cut (m, 0 = off); z = 1 for a depth-Off overlay (KH_VOL_WITNESS: no footprint, no
    // witness; fill_lighting_obj_cb), else 0; w = the first vertex of the run VSUserSo is drawing (KH_USER_VS's
    // pass), or on an effect-chain pass the KH_VM_SEE arm (kh_fx_chain_run: 1 = the depth behind the view model's
    // translucent planes, 2 = also their transmittance); 0 everywhere else.
    float shadow_meta2[4];
    float obj_rot[3][4];   // [0][3] = 1 marks filled (the zeroed default reads as identity
                           // in-shader - auxiliary fills stay correct).
    float blend_ctl[4];   // x = perceptual-composite enable (the flush's fill only; the injection
                          // writes 0), w = the LOD crossfade dither; y / z unread.
    float mat_ctl[4];   // KH_MAT_TABLE (HLSL twin matCtl): x = table index (base + slot) for the
                        // non-instanced VS, y = slot for the instanced VS, z = 1 when x holds for
                        // every instance, w = alpha-mode override (-1 none). The material lanes
                        // themselves live in the table (KhGpuMat). On an effect pass y / z are
                        // KH_FX_SIDE's arms instead (the scene chain's fog scatter / sun flare,
                        // the UI lane's probe), zero unless the pass drew its side value first,
                        // and x / w KH_FX_LINZ's (a scene-chain SSGI / dynamicLightFog / fog
                        // scatter pass's linear-depth copies), zero unless one serves the pass.
    float fuse_meta[4];   // x = the fused stages (kh_fuse_append); y, zw = KH_GLOW_PYR's arm and extent
                          // (kh_glow_lanes), or on an anamorphic pass KH_ANA_PYR's arm and frame (kh_ana_lanes).
    float fuse_stage[12][4];
    // fx2 sits in the append region (append-only block discipline); HLSL twin
    // fxParams2. kh_fill_fx_params_cb copies the object's params 8 - 11 into
    // it (the flush's fills, the PIP's effect pass, the UI lane); the mesh
    // fills of the injection, the PIP and the in-front slice leave the
    // template's zero; a fog-scatter object's fill zeroes it, the chain's
    // fog-scatter pass packs the fog passes into it (kh_fogscatter_pack), and
    // the SSGI resolve writes an auto width into w.
    float fx2[4];
    float dl_ctl[4];   // x = mode (0 off / 1 camera-relative world / 2 view space / 3 absolute
                       // Pool), y = point count, z = spot count, w = distance scale.
    float dl_global[4];   // Xyz = global diffuse multiplier (cb10[3]); w = the global intensity
                          // (g_dl_intensity_bits, 1: no command sets it) - DynLights scales its whole
                          // sum by w, so a zero here is 'lights off', never a padding lane.
    float dl_view[3][4];
    // KH_DL_RING: x = where this draw's light records start in the light ring
    // (t40, HLSL khDlRecs), in float4 elements, as uint BITS (asuint in the
    // shader; exact past 2^24); yzw unused (zero). The records themselves - the
    // engine's 6-float4 cb11 record per light, points first then spots,
    // dl_ctl.y + dl_ctl.z of them - live in the ring, not here, so a mesh takes
    // every light that reaches it with no count cap (kh_dl_select).
    float dl_first[4];
    // KH_USER_LANES (HLSL twin khUserObj): x = the session clock (s,
    // effect_time_seconds), y = this object's creation on that clock (s,
    // fx_t0) - x - y is the object's own age, the effects' fxMeta.y; zw unused
    // (zero). Filled by kh_fill_user_obj_cb from fill_lighting_obj_cb and
    // kh_fill_fx_params_cb (every object fill of a pass that can run a user
    // shader); zero elsewhere.
    float user_obj[4];
    // KH_FX_TEX (HLSL twin khFxTexLay): an effect's textures, 1 + each one's layer in its page ([0].xyzw =
    // user0 .. user3, [1].xy = user4 / user5; 0 = absent or still loading; [1].zw unused). Written by kh_fx_tex_fill
    // on a user effect's draw and (KH_FX_TEX_ALL, kh_fx_tex_on) a builtin effect pass's with textures; zero elsewhere.
    float fx_tex[2][4];
    // KH_FX_FOGREC (HLSL twin khFxFog): the scene's KH fog passes for a user effect - kh_fog_records_pack's two records
    // [startDist, endDist, skyAmount, opacity] (opacity 0 = none), filled for a user effect in the scene chain; zero
    // elsewhere. Last per-object field: the per-frame block (b1) starts at view_proj.
    float fx_fog[2][4];
    float view_proj[4][4];   // Rebased on the two mesh passes (see center_rel); absolute at every other site.
    float inv_view_proj[4][4];   // Inverse of the live encode for the depth reconstruction:
                                 // of the rotation-only view when fx_cam.w = 1 (the shader adds
                                 // fx_cam.xyz back), absolute when fx_cam.w = 0 (KH_FX_CAM_REL).
    float lighting1[4];   // Xyz = unit vector toward the light (engine axes), w = valid flag.
    float lighting2[4];   // Rgb = the engine's sun colour in HDR scene units, copied straight from the
                          // published lighting block (KH_LIGHT_BASE; (1,1,1) before the first publish),
                          // w = shadow-map strength.
    float shadow_meta[4];   // Count, compare sign, bias, atlas size.
    float shadow_tiles[8][4];   // per-cascade atlas UV rects.
    float shadow_mats[24][4];   // per-cascade 4x3 sampling transforms.
    float band_mat[24][4];
    float band_view[24][4];
    float band_border[8][4];
    float cast_mat[3][4];
    float cast_view[3][4];
    // Bands (then the live table) are the receive truth; the mask machinery
    // that remains serves the cast write only, which is registration-exact by
    // construction.
    // x = view-paired band table valid; w = the unit-stencil arm
    // (kh_svs_unit_on), read by the two colour twins. y/z unwritten.
    float mask_meta[4];
    // Fog + engine-mask receive.
    float fog_params[4];   // x = fogValue, y = fogDecay, z = fogBase, w = enable.
    float fog_color[4];   // Rgb = fog color approximation, w = camera altitude ASL (relative).
    // Private sun-depth map (mirrors sunVP/sunMeta in CBFrame).
    float sun_vp[4][4];   // row-vector world -> sun-depth clip.
    float sun_meta[4];   // x = valid, y = map size (px), z = compare bias (normalized depth
                         // units), w = shadow-map strength.
    // x = per-caster locality pair count; y = 1 -> the uncapped t2 list carries
    // them; z = sun map older than 0.5 s (the filtered-compare gate); w =
    // KH_OCC_N, the cast-occupancy grid's edge in texels, so PSMaskCast
    // bounds its Load against the CB rather than a literal; an unwritten 0
    // leaves the grid unarmed (PSMaskCast and kh_cast_scissor alike).
    float locality_meta[4];
    float locality[32][4];   // [2i] = center.xyz, [2i+1] = half extents.xyz.
    float light_amb[4];   // Rgb = engine ambient (HDR) or (1,1,1) cold; w = engine mode.
    float fog_engine[4];   // x = density scale (blk 41), y = fog end (blk 48) z = inv ramp range
                           // (blk 49), w = valid.
    float fog_sky[4];   // Sky row 17 gradient points; w = sky probe valid.
    float fog_sky_col[4];   // Sky row 7 base color (fog color) w = the engine fog-layer altitude
                            // (block row 12 z + row 13 y), the branch point KhHazeT integrates
                            // against.
    float haze_pars[4];   // Engine distance haze (sky CB row 14 = PSC_HazePars): x = reference
                          // Altitude (m), y = density, z = height falloff (/m), w = armed.
    float thm_params[4];
    float thm_meta[4];
    // The volume transport. HLSL twins stenVol / stenVol2 (mirror contract,
    // same relative slot).
    // z/w = the volume copy's own dimensions (not fx_meta.zw). x/y = the copy's
    // depth encode pair (m22, m32) for KH_VOL_WITNESS (kh_fill_sten_reproj;
    // zero = no witness).
    float sten_vol[4];
    // x = transport arm (1 = read the volume stencil); z = KhVsCore vertex path
    // selector (3 = the seam prepass); y/w = the copy's viewport depth range
    // (min, max) for KH_VOL_WITNESS (zero = no witness).
    float sten_vol2[4];
    // Hero sun map - HLSL twins sunVP2 / sunMeta2 (mirror contract).
    float sun_vp2[4][4];   // World -> hero sun-depth clip (row-vector).
    float sun_meta2[4];   // x = valid, y = map size (px), z = compare bias, w = half-diag (m).
    // Frame data - one per pass.
    float sun_vp3[4][4];   // World -> mid-band sun-depth clip (t26).
    float sun_meta3[4];   // x = valid, y = map size (px), z = compare bias, w = half-diag (m).
    float sun_vp4[4][4];   // World -> outer-band sun-depth clip (t27).
    float sun_meta4[4];   // x = valid, y = map size (px), z = compare bias, w = half-diag (m).
    // x = mask valid this frame, y/z = mask dims (px). w is NOT free: it is
    // the sun shadow range, clamp(g_sun_range, 8, 1000) m, written by
    // fill_lighting_frame_cb and by the cast fill whenever a sun anchor
    // exists. Every sun shadow fades out over the last 28% of it
    // (KhSunRangeFade / KhSunRangeFadeAt, which return 1.0 unfaded when the
    // lane reads < 0.5), so zeroing this lane turns the range fade off, it
    // does not free a slot.
    float mir_meta[4];
    float sun_origin[4];   // xyz = the sun anchor; w = the far tier's prefilter arm (KH_FAR_PF).
    // x = engine below-layer extinction (block nb[51]), y = armed; z = the
    // cast fire's frozen-depth ulp, relative (KH_CAST_SNAP_FACING, PSMaskCast
    // only - both mesh fill sites write 0 and no mesh shader reads it); w free.
    float fog_below[4];
    float fog_uw[4];
    float fog_uw_grad[4];
    float snap_vp[4][4];
    float snap_meta[4];
    float snap_cam[4];
    float sun_pf[4];
    // HLSL twin appended identically.
    float sun_vp5[4][4];
    float sun_meta5[4];
    // World cast compare bias, one lane per tier plus the union (HLSL twins
    // sunCastBias / sunCastBias2). Deliberately not sun_meta*[2]: that lane
    // feeds the self kernel too and may not move. Zero = the shader falls back
    // to sun_meta*[2], which every mesh fill site leaves here.
    float sun_cast_bias[4];    // Hero, mid, outer, far (normalized).
    float sun_cast_bias2[4];   // x = union (normalized).
    // No cube matrices live here, and that is the point. Only the spot frustum
    // is arbitrary and therefore stored.
    // xyz = light world position (engine axes), w = far plane (m); w == 0 = no
    // map, the shader answers lit.
    float dls_meta[8][4];
    float dls_ctl[8][4];     // x = 1 spot / 0 cube, y = compare bias constant (metres), z = the
                             // spot's slice, w = bias slope (per metre of range).
    // One float4 per face, indexed slot*6 + face. x = slice for this face (-1 =
    // no slice: nothing occludes in that direction). y = the receiver-normal
    // offset arm, replicated across the light's six entries so it can be read
    // before the face is known. zw free.
    float dls_face_slice[48][4];
    float dls_spot_vp[8][4][4];
    // xyz = pass camera (engine axes), w = clamp(shadowVisibility, 8, 1000); 0
    // = no fade. Written by kh_dls_fill_cb.
    float dls_range[4];
    // Bucket pass lanes (HLSL twins khPass / khPassObj). khPass.xyz = the
    // pass's rebase camera (engine axes), w = 1 arms the instanced vertex
    // path's rebase (the instanced VS subtracts the camera itself). khPassObj.x
    // = the engine object view distance (the object view-distance cut); yzw free. Zero on
    // every fill site that draws no bucket.
    float kh_pass[4];
    float kh_pass_obj[4];
    // xyz = the camera the effect unit's inverse view-projection is relative to
    // (engine axes), w = 1 arms it - KhWorldPosFenced and KhFsFog add xyz back,
    // KhWorldPosRelFenced's readers take camera-relative points. w = 0: the
    // inverse is absolute and nothing is added.
    float fx_cam[4];
    // KH_USER_LANES (HLSL twins khUserCam / khUserCamRot): the pass camera.
    // user_cam.xyz = its position (engine axes, absolute), w = 1 when filled;
    // user_cam_rot rows = its right, up and forward axes (engine axes, unit
    // length), w unused (zero). Filled by kh_fill_user_cam_cb on every pass
    // that can run a user shader (the mesh flush and injection, the PIP and
    // view-model passes, the scene and UI effect chains); zero (w = 0)
    // elsewhere.
    float user_cam[4];
    float user_cam_rot[3][4];
    // KH_VOL_ZOOM (HLSL twin stenVol3): the raster map from this pass's
    // projection to the volume copy's seam-frame projection, per axis
    // copy = raster * xy + zw. The copy was drawn under its seam frame's
    // projection (the injection reads it a frame late), and a zoom between
    // that frame and this pass (a projection change, the view unchanged)
    // moves every surface on screen; through the map the count and the
    // witness depth are read on this pixel's own view ray. x = 0 = identity:
    // zero-initialised fills and the view-model slice (it draws under its own
    // projection). The PIP passes copy the template's map but zero maskMeta,
    // so no stencil is read there.
    float sten_vol3[4];
    // KH_VOL_FOOT (HLSL twin stenVol4): x = 1 when the footprint mask of the
    // copy's seam frame is bound at t33 - the witness then compares against
    // our own footprint's distance there instead of decoding the copy's depth
    // plane. yzw unused (zero). Zero = no mask (the depth-plane witness).
    float sten_vol4[4];
    // KH_PCSS_CELL (HLSL twins sunLat0 / sunLat1): each camera tier's texel
    // lattice offset, as uint BITS (asuint in the shader; mod 2^32) - a texel
    // index of that tier's map plus it is the world-fixed lattice (the floor of
    // the position over the texel along the sun plane's axes) that KhPcssRot
    // hashes. [0] = hero xy, mid zw; [1] = outer xy, far zw. Filled by
    // kh_fill_sun_tiers_cb from the render that made each map; a stood-down
    // tier's pair is unread (its meta is zero).
    float sun_lat[2][4];
    // KH_CB_DERIVED (HLSL twins sunCol / castCam, cb.hlsl's note): never filled by hand - kh_upload_frame_cb forms
    // them from this struct's own matrices at every frame upload (kh_cb_derived), so they cannot drift from what
    // the upload carries. sun_col: each sun map's texel and depth scales, the lengths of its matrix's columns 0 and
    // 2 over rows 0 - 2 - [0] = union xy / hero zw, [1] = mid xy / outer zw, [2] = far xy (zw zero). cast_cam: the
    // frozen mask-cast camera, cast_mat's rows dotted with -cast_view[0].
    float sun_col[3][4];
    float cast_cam[4];
};
                                           // (ensure_resources).
// The ring binds the same buffer every slice with only firstConstant moving -
// the shape the D3D11.1 docs warn about under command-list emulation, where an
// offset-only SetConstantBuffers1 may not apply and a null bind between two
// Set1 calls is the workaround. The trigger is a deferred context on a driver
// without native command lists; every upload here uses the immediate context.
inline ID3D11DeviceContext1* kh_ctx1(ID3D11DeviceContext* ctx);
static bool     g_kh_ctx1_null_first = false;
// A driver without offsetting drops SetConstantBuffers1, so StateBackup's
// offset-aware restore must not be attempted there or our buffer stays bound.
// False until ensure_resources reads the caps.
static bool     g_cb_offsetting = false;
inline uint64_t steady_now_ms();
inline void  mul_4x4(const float a[4][4], const float b[4][4], float out[4][4]);

inline std::string hr_str(HRESULT hr) {
    char b[16];
    sprintf_s(b, "0x%08X", static_cast<unsigned>(hr));
    return std::string(b);
}
inline KhChainParams kh_chain_defaults();   // Defined in rendering_integration_features.hpp.

// One simulated instance. Owned by shared_ptr so a worker holding a job keeps
// it alive past the script removing the mesh.
struct KhClothInst {
    KhClothState st;
    // Render vertex -> particle. Many-to-one: a UV or smoothing seam splits a
    // render vertex without splitting the cloth, which is the difference
    // between a sheet and a sheet that tears along every seam.
    std::vector<uint32_t> vmap;
    std::vector<MeshVertex> rest;      // The mesh's own vertices, the fallback and the LOD tail.
    std::vector<MeshVertex> out[2];    // Double buffered deformed array.
    std::vector<float> nrm_acc;        // Normal accumulator, 3 per render vertex.
    std::vector<float> tan_acc;        // Tangent accumulator, 3 per render vertex (kh_cloth_writeback).
    // Render vertex -> the render vertex whose accumulator it shares: the
    // first one on the same particle with the same authored normal (see
    // kh_cloth_build). Written at build, read by the writeback.
    std::vector<uint32_t> nrm_rep;
    // Build-time only, and deliberately not in KhClothPart, which the solver
    // touches every substep: the authored normal of each particle, 3 per
    // particle, used once to decide which surfaces face each other.
    std::vector<float> part_nrm;
    // KH_SKEL, written at build. part_inf: each particle's four bone
    // influences, from the first render vertex on it (every render vertex at
    // one position carries the same ones); empty when the mesh has no vertex
    // groups. auth_ctr / auth_ext: the mesh's native_ctr / native_ext, so a
    // stored rest position r was authored at r * auth_ext + auth_ctr.
    std::vector<KhSkinInf> part_inf;
    float auth_ctr[3] = { 0.0f, 0.0f, 0.0f };
    float auth_ext[3] = { 1.0f, 1.0f, 1.0f };
    uint32_t mesh_edges = 0;           // Stretch entries that are real mesh edges; shell links follow.
    float    shell_built = -1.0f;      // The 'shell' the current links were built for.
    uint32_t lvl0_icount = 0;
    int      mesh = -1;
    uint32_t slot = 0xFFFFFFFFu;
    // The object this instance simulates, by its creation stamp (RenderObject::
    // seq, unique per object within a mission and carried through every
    // update). Slot and mesh alone do not name an object: a slot is reused, and
    // a new object on it with the same mesh would take over the old drape.
    uint64_t seq = 0;
    // Written by the game thread under g_cloth_mu (the upload reads it under the
    // same lock). Dead, but a worker still held it when kh_cloth_sync came to
    // retire it: never stepped, never installed in a slot again, and erased the
    // first frame it is free. Also set on the erase itself
    // (KH_CLOTH_ERASE_RETIRE), for the upload's snapshot.
    bool retired = false;
    // THE FLOATING ORIGIN (engine axes, metres, double): the particles, the
    // carrier frame and the colliders' views are all stored RELATIVE to it.
    // Game thread only, on a non-busy instance (kh_cloth_sync).
    double org[3] = { 0.0, 0.0, 0.0 };
    bool   org_on = false;
    KhClothParams par = kh_cloth_defaults();

    // The frame's inputs, written by the game thread while 'busy' is false.
    float in_centre[3] = {};
    float in_rot[9] = {};
    bool  in_rotated = false;
    float in_size[3] = { 1.0f, 1.0f, 1.0f };
    float in_dt = 0.0f;
    float in_wind[3] = {};
    // Every collider in reach, however many.
    std::vector<KhPhysicsColliderView> in_col;
    // The BVHs the views point at, held for the duration of the step so a
    // collider mesh released mid-step cannot free the geometry under a worker.
    std::vector<std::shared_ptr<const KhPhysicsBvh>> in_col_own;
    // KH_AFFECTOR: the affectors in reach this frame, already in this
    // instance's frame, written with the rest of the inputs above.
    std::vector<KhClothAffView> in_aff;
    // Game thread only: the frame time that passed while a worker still held
    // this instance, owed to its next step - a skipped frame is late, not lost.
    float dt_owed = 0.0f;
    // KH_CLOTH_GROUND: the terrain under everything this cloth can reach this
    // frame, written with the rest of the inputs (kh_cloth_ground_patch).
    KhClothGround in_ground;
    // KH_SKEL: the skeleton's pose, one affine map per bone in kh_skin_affine's
    // layout (empty = no guide), and whether the object is a skeletal binding:
    // one steps in its REST box at the model's origin (kh_cloth_sync) and has
    // its buffers written in the box of the drawn shape (kh_cloth_box_out).
    std::vector<float> in_guide;
    bool in_boxed = false;
    // KH_SIM_LOD (game thread, kh_cloth_sync, written with the inputs): the
    // iterations and substeps this step runs at - the object's own dials
    // scaled by the LOD share - and, standing between frames, the
    // hysteresis's memory of the last choice. 0 = not chosen yet (the job
    // takes the dials themselves). Read by kh_cloth_run.
    uint16_t in_iter = 0;
    uint16_t in_sub = 0;

    std::atomic<bool>     busy{ false };
    std::atomic<uint32_t> front{ 0 };   // Which out[] the upload may read.
    std::atomic<uint32_t> gen{ 0 };     // Bumped on every publish; the upload's dirty test.
    uint32_t uploaded_gen = 0xFFFFFFFFu;
    // KH_CLOTH_SHAPE: the shape key of each out[] buffer (kh_cloth_shape_note)
    // and of the one the upload last wrote, which is what g_cloth_gen_slot
    // publishes. shape_of[b] is written by the worker before front's release
    // store and read by the upload after its acquire load, like out[b].
    uint32_t shape_of[2] = { 0u, 0u };
    uint32_t uploaded_shape = 0u;
    // KH_SKEL: when box_on[b], out[b] is stored in the box box_ctr[b] (its
    // centre in the authored frame) / box_size[b] (edge lengths), engine axes,
    // metres; otherwise in the frame's own rest box. Written by the worker
    // before front's release store and copied by the upload with the buffer,
    // like shape_of.
    bool  box_on[2] = { false, false };
    float box_ctr[2][3] = {};
    float box_size[2][3] = {};
    bool  uploaded_box_on = false;
    float uploaded_box_ctr[3] = {};
    float uploaded_box_size[3] = {};
    // KH_CAST_REFIT (a plain instance, neither boxed nor a chain): the edge
    // lengths out[b]'s level-0 vertices reach about the centre, in the frame's
    // own space (engine axes like st.size, metres, >= the frame's size). Published as box_*
    // are; the upload hands the object the uploaded buffer's.
    float cast_size[2][3] = {};
    float uploaded_cast_size[3] = {};
    // Worker-private: a boxed instance's writeback target in the frame's own
    // space, which out[] is then mapped from.
    std::vector<MeshVertex> sim_out;
    // Worker-private: the mesh-local positions (3 per render vertex) of the
    // shape that took the current key, and that key.
    std::vector<float> shape_ref;
    uint32_t shape_key = 0u;
    // Created by kh_cloth_upload (flush_locked's housekeeping: the render
    // thread's scene flush, or the game thread under a park); retired to
    // g_cloth_grave by kh_cloth_sync; released by the upload's grave drain or
    // kh_cloth_release_all. Never touched by a worker.
    ID3D11Buffer* vb = nullptr;
    bool built = false;
    // KH_CHAIN: this instance is a chain (kh_chain_build), not a cloth. It
    // keeps the carrier's frame, the substep clock and the counters in st,
    // which the shared plumbing reads (kh_cloth_sync, kh_cloth_ground_patch,
    // kh_cloth_origin_shift, kh_cloth_upload), and its bodies in ch. cpar and
    // in_tg are inputs like the rest (written while busy is false); ch_start /
    // ch_end are the bone names it was built for, so new ones rebuild it.
    // sim_met is worker-private: the skinned positions, carrier-local metres.
    bool chain = false;
    KhChainState ch;
    KhChainParams cpar = kh_chain_defaults();
    KhChainTarget in_tg;
    std::string ch_start;
    std::string ch_end;
    bool ch_start_head = false;   // KH_CHAIN_POINT: the anchor and hold points it
    bool ch_end_tail = false;     // was built for; a change rebuilds it too.
    std::vector<float> sim_met;
    // KH_CLOTH_PIN_FRESH - what the guide alone decides (the cloth's hard pins,
    // the chain's kinematic bodies and guide-mapped bones) is re-placed from
    // this frame's guide into the front buffer's frame on every upload, whether
    // or not a job landed, so the bound part never lags a slow simulation.
    // ch_mat[b]: the per-bone maps kh_chain_writeback skinned out[b] from (12
    // floats a bone), written with out[b]. pin_out / pin_mat: scratch for
    // whichever re-places the bound part (kh_cloth_upload or the consume-point
    // step), under g_cloth_mu. pin_guide: the upload's copy of the Draw3D
    // guide.
    // KH_CLOTH_PIN_STEP - pin_step_cycle: the cycle whose consume-point step
    // re-placed the bound part and wrote vb; the upload leaves that cycle's
    // bytes, key and box standing.
    uint64_t pin_step_cycle = ~0ull;
    uint32_t pin_front = 0;   // KH_CHAIN_END_DRAW: the out[] buffer that step placed (its pin_mat's).
    // KH_CHAIN_END_DRAW - per out[] buffer, written by the writeback before front's release like box_*: the end's
    // position and rotation targets the job held, carrier-local (false: that lane had none). end_mat / end_out /
    // end_met: kh_chain_end_draw's scratch; end_shape_ref / end_shape_key: the shape that took the key of the bytes
    // it writes (KH_CLOTH_SHAPE's rule, kh_cloth_step_shape). Render thread, under g_cloth_mu.
    bool  end_job_p[2] = { false, false };
    bool  end_job_r[2] = { false, false };
    float end_job_x[2][3] = {};
    float end_job_q[2][4] = {};
    std::vector<float>      end_mat;
    std::vector<MeshVertex> end_out;
    std::vector<float>      end_met;
    std::vector<float>      end_shape_ref;
    uint32_t                end_shape_key = 0u;
    // The same targets in the world (engine axes; a rotation as its quaternion), for an end fixed there: its frame
    // target is where the job held it (KH_CHE_WORLD, KH_CHE_HOLD). end_anc: kh_chain_end_fix's scratch.
    double end_job_w[2][3] = {};
    float  end_job_wq[2][4] = {};
    std::vector<int32_t>    end_anc;
    // KH_CHAIN_END_DRAW (C3) - vb_wr counts every write of vb (kh_cloth_upload, kh_cloth_pin_draw_step,
    // kh_chain_end_draw); the draw's last write: vb_wr then (end_wr), the buffer, front and generation, the bone
    // maps it skinned and the front's box - equal again, with no write between, the bytes would be the same, and
    // the draw keeps them (end_be: the box they were stored in). Render thread, under g_cloth_mu.
    uint32_t                vb_wr = 0u;
    uint32_t                end_wr = 0u;
    bool                    end_wr_ok = false;
    ID3D11Buffer*           end_vb = nullptr;
    uint32_t                end_f = 0u, end_gen = 0u;
    std::vector<float>      end_mat_last;
    float                   end_box[6] = {};
    float                   end_be[3] = {};
    // KH_CHAIN_END_NOTE: the cycle (g_topo_cycles) of the draw's last write - kept bytes written in this one
    // are still news to a final or repick step (kh_chain_end_draw's note).
    uint64_t                end_new_cyc = ~0ull;
    // KH_CLOTH_STEP_SHAPE - pin_g: the guide kh_cloth_pin_place last re-placed a chain from (kh_chain_end_draw reads
    // the pin step's: an end held on a bone). pin_met: kh_cloth_pin_seal's scratch; pin_shape_ref / pin_shape_key: the
    // shape that took the key of the re-placement's bytes. Under g_cloth_mu (render thread, or a park).
    std::vector<float>      pin_g;
    std::vector<float>      pin_met;
    std::vector<float>      pin_shape_ref;
    uint32_t                pin_shape_key = 0u;
    // KH_CLOTH_PIN_GRADE - out_guide[b]: the guide the job solved out[b]
    // against (its in_guide, copied with out[b]). The re-placement above was
    // binary - a hard pin took this frame's bones, a particle painted 2 % sim
    // kept the pose an older guide gave it - while the solver's gradient is
    // continuous, so the cloth tore along the pin boundary by however far the
    // bones had moved since the job's guide (a frame or more: the job is
    // asynchronous, and KH_SIM_LOD spaces its steps). With it kh_cloth_pin_place
    // carries a gradient particle along by (1 - its paint weight) of what its
    // OWN target moved since - 1 at the pinned end, 0 at the simulated end, so
    // it meets both neighbours. Empty or of another bone count = no grading.
    std::vector<float> out_guide[2];
    std::vector<float> ch_mat[2];
    std::vector<MeshVertex> pin_out;
    std::vector<float> pin_guide;
    std::vector<float> pin_mat;
};

static std::mutex g_cloth_mu;                                            // Guards the table and the queue.
static std::unordered_map<uint32_t, std::shared_ptr<KhClothInst>> g_cloth;   // By scene slot.

// Slot -> substitute vertex buffer, consulted by every draw pass. Resized only
// by the uploads in flush_locked's housekeeping (kh_cloth_upload,
// kh_skin_upload, kh_uvs_upload); elements written only there, by
// kh_cloth_release_all, or by kh_cloth_slot_forget (kh_scene_sync,
// kh_uvs_release_all) - never while a render-thread reader is mid-read. A
// buffer is released and its element cleared in the same window.
static std::vector<ID3D11Buffer*> g_cloth_vb_slot;
// KH_SKIN_POOL: some element of g_cloth_pool_slot may name a chunk -
// kh_skin_upload sets it after its installs, kh_skin_release_all and the
// session clear it; under the tables' rules. The depth routes' tie-break and
// test (kh_cast_pool_key, kh_cast_pool_of) skip the tables without one.
static bool g_skin_pool_any = false;
// The published SHAPE KEY per slot (KH_CLOTH_SHAPE; 0 = no cloth), beside the
// buffer and under the same lifetime rules - except that the render thread's
// consume-point writers (kh_skin_draw_finish, kh_cloth_pin_draw_step,
// kh_chain_end_draw, kh_uvs_step) also write an element, the key following the
// bytes they wrote. A cloth instance's BUFFER POINTER
// never changes while its CONTENTS change every frame, so any pass that caches
// on "has this draw changed" has to hash this and not the pointer - otherwise
// a moving cloth keeps a footprint or a shadow map rendered from a shape it
// has left. The key moves only when the drawn shape does, so a cloth at rest
// costs those passes nothing. Hashed by kh_volume_seam_inject,
// render_sun_depth and kh_dls_render.
static std::vector<uint32_t> g_cloth_gen_slot;

// KH_CHAIN - what the script set with chainSimulation, by handle. GAME THREAD
// ONLY: written by the command, read by kh_chain_prepare and kh_cloth_sync,
// never by the render thread. It lives here rather than in RenderObject
// because it holds game objects - an end that follows an object or a memory
// point - and a RenderObject is copied to the render thread, where no
// game_value may be copied; the one bit the render thread needs,
// RenderObject::chain_sim, is there. Like the cloth's parameters it OUTLIVES
// the simulation (switching the chain off and on keeps the tuning and the
// end), and it dies with its object: kh_chain_prepare drops an entry whose
// handle is gone or names another object, and hands its proxy to the delete
// queue; a mission's end clears the table (reset_retained_state).
//
// The END's two lanes are independent, as a mesh's position and rotation are.
// pos: FREE (not held), BONE (true: the end bone's place on the carrier - its
// memory point on a bound skeleton, else its authored place in the mesh's
// frame), HOLD (false: where it stood when the chain started), WORLD ([x, y,
// zASL]), OBJ (an object's position), MEM ([object, memoryPoint], through a
// KH_ATTACH_BONE proxy, read as a mesh attached to one is). rot: the same
// kinds; true resolves at the command to the rotation of whatever the
// position follows (MEM, OBJ) or BONE.
enum KhChainEndKind : uint8_t { KH_CHE_FREE = 0, KH_CHE_BONE, KH_CHE_HOLD, KH_CHE_WORLD, KH_CHE_OBJ, KH_CHE_MEM };
struct KhChainRead { uint32_t off = KH_ATTACH_OFF_NONE; uintptr_t vb = 0; uintptr_t bb = 0; uintptr_t avb = 0; uintptr_t abb = 0; };
struct KhChainCfg {
    uint64_t seq = 0;                   // The object this entry belongs to (RenderObject::seq).
    KhChainParams par = kh_chain_defaults();
    std::string start;                  // Lower-case bone names; "" = the default (KhChainState).
    std::string end;
    // KH_CHAIN_POINT - which point of each named bone the chain uses. The
    // start's ANCHOR: false = its tail, the first simulated bone's head, which
    // is where a chain has always hung from; true = the start bone's own head.
    // The end's HELD point: false = its head, the joint anchor, likewise the
    // default; true = its tail. Both defaults leave the build bit-identical.
    bool start_head = false;
    bool end_tail = false;
    uint8_t pos = KH_CHE_FREE;
    uint8_t rot = KH_CHE_FREE;
    bool rot_true = false;              // rot was given as true: resolved against pos (kh_chain_cfg_resolve).
    float pos_w[3] = { 0.0f, 0.0f, 0.0f };   // WORLD: SQF [x, y, zASL].
    float rot_w[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };   // WORLD: rot_m rows, engine axes.
    game_value pos_obj;                 // OBJ: the object. MEM: the memory point's owner.
    game_value proxy;                   // MEM: the proxy on the memory point; ours to delete.
    game_value rot_obj;                 // rot OBJ: the object.
    KhChainRead rd_pos, rd_rot, rd_par; // Raw-read caches (kh_attach_raw).
    // The last transform each followed lane read, and whether it has read one.
    // A raw read can refuse for a frame (the page gate, a basis that fails the
    // checks) while the object is perfectly alive; the lane then HOLDS this
    // rather than going free, which is what kh_attach_raw's other callers do
    // with a refused read - a socketed end must not fall out of its chute for
    // one frame and creep back at the approach rate. A lane re-pointed at
    // something else drops it, and a dead object still frees the lane.
    bool   read_p = false, read_r = false;
    double last_pos[3] = { 0.0, 0.0, 0.0 };
    float  last_rot[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    // This frame's reads (kh_chain_prepare): world, engine axes.
    bool   live_p = false;
    bool   live_r = false;
    double live_pos[3] = { 0.0, 0.0, 0.0 };
    float  live_rot[9] = { 1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f };
    // KH_CHAIN_REFUSED_END: the chain builds nothing - its mesh, loaded, carries no skeleton (kh_chain_mesh_ok), or
    // its build was refused (kh_chain_refused). Written by kh_chain_prepare's pass ahead of kh_chain_end_publish,
    // its one reader, in the same pass. Its ends are still read there as before: the lanes' hold (read_p /
    // last_pos) stays current for the frame the chain builds again.
    bool   nobuild = false;
};
static std::unordered_map<std::string, KhChainCfg> g_chain_cfg;

// The world wind, ENGINE axes (east, up, north), sampled once per Draw3D by
// kh_cloth_wind_sample and read by the sync a few lines later. One frame of
// age is not worth a second engine call: wind is a slow field and the cloth
// it drives is already published a frame behind the draw.
static float g_cloth_wind[3] = { 0.0f, 0.0f, 0.0f };

// KH_CLOTH_GROUND - the terrain the cloth lands on. The source is the
// framework's terrain matrix (g_terrain_matrix, built by
// initialize_terrain_matrix, which kh_thm_autobuild_step calls from
// flush_frame ahead of the cloth step, on a mission's first frame with
// anything to draw): the engine's own height at every terrain-grid node, rows
// north and columns east from the map's corner, every g_terrain_grid_width
// metres. Between the nodes the engine's ground is two flat triangles per
// cell, split along one diagonal or the other; which one is MEASURED here,
// once per matrix and mission, by asking the engine for the height at the
// centre of a few cells whose two diagonals disagree - the centre lies on
// both, and only one answer is the engine's. The four parity classes of a
// cell are measured apart, so a checkerboard or row-alternating split is
// found as well as a uniform one. A class that answers neither way falls back
// to a smooth (bilinear) fit and says so once. Game thread only; the workers
// see only each cloth's own copy of the nodes under it. The heightfield our
// meshes are clipped against takes the same split (KH_THM_EXACT:
// kh_thm_autobuild_step measures it here too, with or without a cloth).
static bool     g_ground_ready = false;              // A matrix and its split are known.
static float    g_ground_cell = 0.0f;                // The matrix measured: its spacing...
static size_t   g_ground_n = 0;                      // ...and its nodes per side.
static uint8_t  g_ground_split[4] = { 2, 2, 2, 2 };  // KhClothGround::split, by class.

// KH_SKEL runtime - a skeletal mesh's vertices, skinned from its parent's
// memory-point proxies and published through the cloth's slot tables, so no
// pass or shader knows skinning exists.
//
// THE MODEL. Everything is in the parent's frame (engine axes, metres, the
// parent's visual origin). A proxy carries its bone's animated transform
// relative to the parent: rotation Q and position t. A bone matching a proxy's
// memory point maps v to (v - head) * Q + t (identity at rest: the mesh's bones
// sit on the memory points). An unmatched bone takes its nearest matched
// ancestor's map (or the root's identity). Vertices blend four influences
// (linear blend skinning); missing weight goes to the root.
//
// THREADS - the cloth's protocol on the cloth's pool. kh_skin_sync (game
// thread) reads the proxies under g_draw_list_mutex, builds one affine map per
// bone and hands the instance to a worker ('busy'). The worker (kh_skin_job)
// skins into the back buffer from the instance's immutable rest snapshot,
// writes that buffer's box, shape key and gen (gen_of), release-stores front
// and gen, and clears busy last. kh_skin_upload acquires gen (anything new?),
// then front, and installs that buffer with its own box, key and gen_of - a
// gen read just before a job's front store names the result before it
// (KH_SKIN_GEN_OF). One job per instance at most, and the upload walks with
// g_cloth_mu held, under which every job leaves the queue, so no job can start
// on the buffer it reads (the one a running job writes is the other). g_skin_gpu
// is under g_skin_mu (the engine-reset release can run on the render thread).
//
// THE BOX. A skinned buffer is stored in its own pose's box, and the object's
// size and skel_ctr are set only where the buffer is installed (the upload and
// the consume-point steps).
//
// NORMALS. n / native_ext is the authored normal; written back as n * box it is
// what KhVsCore's normalize(n / size) expects.
//
// CLOTH. A binding whose mesh simulates is the cloth's to draw: no skinning job
// runs; kh_skin_sync keeps its pose as per-bone maps (KhSkinInst::guide) for
// kh_cloth_sync, whose bones place the pinned part. kh_skin_upload hands the
// object the box of whichever buffer its slot ends up with.
//
// COLLIDER. The job that skins a pose also builds its BVH
// (kh_physics_bvh_build, metres about the pose's box centre) and publishes it
// with the buffer; kh_skin_upload records the installed result's BVH
// (g_skin_col) for kh_physics_gather_colliders. Each pose carries its corners'
// motion since the previous pose (KhPhysicsBvh::vd; none when the chain was
// cut), stamped at its first install (KhSkinGpu::serial), so exactly the next
// gather sweeps it across its substeps. The sweep is discrete: a limb moving
// more than half its thickness in one substep can tunnel. A hidden collider
// still collides.
static constexpr float KH_SKEL_REACH_M = 500.0f;   // A proxy farther than this from its parent is not yet attached.

// KH_SKIN_POOL - where the GPU-skinned poses live: per (mesh, slice size) a
// pool of chunks, each one DEFAULT buffer (VERTEX_BUFFER | STREAM_OUTPUT |
// SHADER_RESOURCE, with a R32G32B32A32_UINT view - three elements a
// MeshVertex) of 'used.size()' equal slices, one per binding (KhSkinGpu::
// chunk / slice / off). The GPU skins into the slice (stream output at its
// offset), the CPU writes it with a box, every pass binds it with the IA
// offset, and the instanced routes read it through the view at the base the
// record carries (KhObjRec::res.w) - many bindings' poses, one buffer, one
// draw. The memory follows the population, as the bindings' own buffers did,
// within a factor: a flush's new slices are reserved at once
// (kh_skin_pool_reserve: the last chunk grown to fit, then whole chunks); a
// take past them grows the last chunk in place, doubling (kh_skin_pool_grow: a
// new buffer, the old one copied to its start, so no slice moves) up to
// KH_SKIN_POOL_CHUNK_BYTES (or one slice when a slice is larger), then a new
// one-slice chunk starts; a pool whose live slices fall to a quarter of its
// slices is repacked into half that (kh_skin_pool_trim: fresh chunks, every
// slice copied, every base new); a chunk goes when its last slice does, or at
// the next trim when nothing took it. So a pool holds between one and four times its live
// slices (rounded up to a whole chunk), and a mesh's bindings share as few
// chunks - instanced draws - as 64 MB allows. A binding whose mesh changes at
// the same byte size leaves its slice (kh_skin_upload), so a chunk holds one
// mesh's poses. Only a stream-output buffer is pooled (the CPU's DYNAMIC one
// never is); a chunk whose buffer or view the device refuses leaves the
// binding a buffer of its own, as before, for as long as that buffer stands
// (the reservation counts it as housed: it rejoins a pool only once the
// buffer goes - its binding ends or changes byte size, the device or the
// session goes) - and when the device has made no
// pool buffer yet, it refuses the kind itself, which stops further pooling
// until the device goes (KH_SKIN_POOL_MADE: a refusal after one stood is that
// take's alone); a refused growth or repack keeps what stands. Under g_skin_mu;
// created, grown, repacked and released only by kh_skin_upload (the flush's
// housekeeping, after kh_cloth_upload emptied the slot tables - a growth there
// re-points the elements the walk installed already: kh_skin_pool_rebind) and
// kh_skin_release_all (beside kh_cloth_release_all), so no table element names
// a released buffer.
struct KhSkinPoolChunk {
    ID3D11Buffer* vb = nullptr;
    ID3D11ShaderResourceView* srv = nullptr;
    uint64_t key = 0u;                  // Its pool's (g_skin_pool).
    uint32_t serial = 0u;               // Unique for the session and never 0: the instanced routes' key lane.
    std::vector<uint8_t> used;          // Per slice.
    uint32_t live = 0u;                 // Slices in use.
};
// KH_TEX_LAND: bumps when kh_tex_async_pump places a pending texture. A cutout / blend caster drew solid
// while its map was pending; the shadow skip keys (sun union and tiers, DLS, seam) mix this in while a
// textured alpha caster is in their set, so the next pass redraws with the map. Not g_tex_page_gen, which
// also bumps on frees and grows that change no map. kh_tex_resolve's synchronous placement needs no bump:
// it places inside the ensure of the draw that first resolves the map. Same serialized window as
// g_tex_page_gen (render thread, or the game thread under the park).
static uint32_t g_tex_land_serial = 0;
// Defined in rendering_integration_features.hpp.
inline void kh_fx_tex_bind(ID3D11DeviceContext* ctx, ID3D11ShaderResourceView* const khft_srv[KH_MAT_USER_MAPS]);
inline void kh_fx_tex_unbind(ID3D11DeviceContext* ctx);   // Defined in rendering_integration_features.hpp.
// The pages bound for a scope and taken off at its end, on every exit: the scene chain's pass (its side draws return
// early on a failed upload) and the UI lane's deferred builtin pass.
struct KhFxTexScope {
    ID3D11DeviceContext* ctx = nullptr;
    KhFxTexScope() = default;
    KhFxTexScope(const KhFxTexScope&) = delete;
    KhFxTexScope& operator=(const KhFxTexScope&) = delete;
    void bind(ID3D11DeviceContext* khts_ctx, ID3D11ShaderResourceView* const khts_srv[KH_MAT_USER_MAPS]) {
        kh_fx_tex_bind(khts_ctx, khts_srv);
        ctx = khts_ctx;
    }
    ~KhFxTexScope() { if (ctx) kh_fx_tex_unbind(ctx); }
};
// Two writers: flush_locked and inject_composited_meshes both publish it - on
// the render thread, or in a park's flush on the game thread with the render
// thread held (a frame the flush never publishes on would otherwise draw
// every object at level 0), with the same formula from their own pass. Read
// lock-free by kh_lod_pick. KH_PIP_LOD: a pass with a camera and a target of
// its own (the PIP) hands kh_lod_pick its own value instead - this one is the
// main view's.
static float g_lod_proj_px = 0.0f;

struct KhCullSet {
    float p[4][4] = {};   // Left, right, bottom, top - normalised.
    float bias[3] = {};   // Add to a world centre to reach the matrix's space.
    bool  valid = false;
    // The four side planes pass through the apex, so they bound a double cone -
    // the frustum and its mirror behind. "Is this object behind the camera"
    // must not be asked with a near plane from the matrix's column 2 (depth
    // encoding is unrelated).
    float fwd[3] = {};
    float apex[3] = {};
    bool  back_ok = false;
    bool  back_on = false;
};

// KH_VIS_OCC: conservative CPU occlusion culling for the injection colour pass
// (the one pass that depth-writes every solid). Occluders are the opaque solids
// of any mesh with a KH_OCC_BOX (kh_mesh_occ_prep) that draw that mesh as it
// was proven (kh_occ_drawn_as_built), plus the objects a script declared
// occluders (KH_OCC_FORCE, drawn or not). Their boxes' front faces, each
// clipped at the near depth (KH_VOC_NEAR_CLIP), rasterize
// inner-conservatively into a small grid of occluder depths at their
// farthest-corner camera distance, and a cell an edge between two of them
// crosses counts when the two together cover it (KH_VOC_UNION). An object is
// hidden when every cell its
// projected AABB touches holds an occluder strictly nearer than its nearest
// possible point. Every guard fails toward 'visible'. Occluders enter
// regardless of their own verdict (hiding is transitive).
static constexpr int   KH_VOC_W = 96;
static constexpr int   KH_VOC_H = 54;
static constexpr float KH_VOC_FLOOR_REFUSE = 3.0e38f;

struct KhVisOcc {
    float z[KH_VOC_W * KH_VOC_H];   // Farthest-corner camera distance of the nearest covering face.
    float vp[4][4];
    float bias[3];   // KhCullSet::bias - world into the matrix's space.
    float cam[3];
    float margin = 0.0f;   // Euclidean metres of extra separation the cull demands: the far-arb
                           // clamp's view-depth cap times the frustum-corner secant (call site).
    bool  on = false;
};
static std::atomic<uint64_t> g_ref_note_cen_ms{ 0 };   // KH_GC_NOTE_SKIP: when the census last stamped (GetTickCount64).

static std::atomic<bool> g_mesh_publish_pending{false};

// One KhInstLane per instance (16 B; HLSL VSInst, IA slot 1, TEXCOORD4 uint +
// TEXCOORD5 float3): the live-scene slot (the VS reads the record from t39),
// the LOD dither, the final alpha (envelope applied) and the material table
// base. Two dynamic rings, one per pass thread, discard on first use and wrap,
// NO_OVERWRITE per write. The sun-depth and DLS passes consume the same lane
// through their own ring.
struct KhInstLane {
    uint32_t slot;
    float    dither;
    float    alpha;
    float    mat_base;
};
static_assert(sizeof(KhInstLane) == 16, "KhInstLane is one float4 (HLSL VSInst twin)");
static constexpr uint32_t KH_INST_STRIDE = static_cast<uint32_t>(sizeof(KhInstLane));

// The bucket key: every object field a bucket's shared constant block or
// pipeline state is filled from. What varies per instance rides the record or
// the lane and is deliberately absent here.
struct KhInstKey {
    int      mesh;
    uint32_t pool;      // KH_SKIN_POOL: the serial of the skin pool chunk the bucket pulls from (0 = the mesh's own
                        // buffer, bound on slot 0); kh_inst_plan_build sets it from kh_inst_eligible's chunk.
    uint64_t mats;      // 0 = untextured; else the set's page signature (KhMaterialSet::Gpu).
    int      blend;
    int      mode;
    int      lit;
    int      two_sided;
    int      part;
    uint32_t route;     // The pass's per-object route bits (2 near-gap) - CB lanes, PS and
                        // viewport.
    uint64_t aux;       // The object's dynamic-light selection identity (kh_dl_bucket_hash; 0 when
                        // the selection is not per object): fill_dynlights_cb culls the pool to the
                        // mesh's nearest set in mode 3, and the bucket uploads the representative's.
};

// One emitted instance range: a bucket's members at one LOD level, contiguous
// in the ring from lane 'first' (ring-absolute), 'n' lanes.
// src = the range's first lane in the plan's own stream (p.lanes / p.lane_d2), before the ring offset.
struct KhInstRange { int lvl; uint32_t first; uint32_t n; uint32_t src = 0; };
// KH_INST_PART_ORDER: one lane of a translucent part's far-to-near walk (kh_inst_draw, part 2).
struct KhInstPartLane { float d2; uint32_t lane; int lvl; };

struct KhInstPlan {
    std::vector<int32_t>  batch_of;      // Per mesh index: bucket id, -1 = draws per object.
    std::vector<uint8_t>  done;          // Per mesh index: carried by a bucket (or culled).
    std::vector<uint32_t> batch_begin;   // Per bucket: first member in 'members'.
    std::vector<uint32_t> batch_n;       // Per bucket: member count.
    std::vector<const KhSkinPoolChunk*> batch_pool;   // KH_SKIN_POOL, per bucket: the chunk its Sk twin pulls from
                                                      // (null = the mesh's own buffer).
    std::vector<uint32_t> members;       // Mesh indices, bucket-contiguous, visiting order within a
                                         // Bucket.
    struct Ent { KhInstKey key; uint32_t rank; uint32_t idx; const KhSkinPoolChunk* pool_c; };
    std::vector<Ent>      ents;
    // The emission: per bucket the representative (the first visible member in
    // visiting order; the loop draws the bucket when it reaches it) and its
    // ranges.
    std::vector<uint32_t> rep;           // Per bucket; KH_SCENE_NONE = nothing visible.
    std::vector<uint32_t> range_begin;   // Per bucket: first range.
    std::vector<uint32_t> range_n;       // Per bucket: range count.
    std::vector<KhInstRange> ranges;
    std::vector<std::pair<uint64_t, uint32_t>> picked;   // Scratch: (lod << 32 | far-to-near
                                                         // Rank, mesh index) with the dither
                                                         // Sign folded into bit 31.
    std::vector<KhInstLane> lanes;       // Scratch: the pass's whole stream, one write.
    std::vector<float>    lane_d2;       // Scratch, per lane of 'lanes': its member's distance (the pass's
                                         // memo) - KH_INST_PART_ORDER's sort key.
    mutable std::vector<KhInstPartLane> part_order;   // Scratch of kh_inst_draw's part-2 walk.
    std::vector<float>    lod_t;         // Scratch, per mesh index: the member's own crossfade
                                         // fraction from its one pick (0 = none).
    uint32_t lanes_written = 0;
};

static ID3D11VertexShader* g_vmir_vs = nullptr;

// Scene HDR colour capture: the bound RTV cannot be sampled while bound, so the
// scene is resolved (MSAA) or copied into an owned single-sample texture + SRV.

// KH_XTHREAD_ATOMIC: written by setSsgiScale (game thread), read by ensure_fx_chain (render thread or a park).
static std::atomic<float> g_khsg_scale{ 0.5f };
static std::atomic<float> g_sun_range{200.0f};
static std::atomic<float> g_obj_vis{0.0f};

// Pipeline state snapshot / restore (only what we touch). Constant buffers are
// per-stage: b0/b1 of both VS and PS.

// Restores itself if the pass that captured it unwinds before its explicit
// restore (an exception inside a hook or the flush): the host's pipeline state
// is never left as ours, and no captured reference leaks. The explicit
// restore() stays the normal path; the destructor is the exception path only.
struct StateBackup {
    ID3D11DeviceContext*     ctx_saved = nullptr;   // The context of the capture (the destructor's).
    bool                     armed = false;         // Captured and not yet restored.
    ID3D11InputLayout*       input_layout = nullptr;
    D3D11_PRIMITIVE_TOPOLOGY topology = D3D_PRIMITIVE_TOPOLOGY_UNDEFINED;
    // Two IA vertex slots: the instance stream binds slot 1. A slot we touch is
    // a slot we save; widen in the same edit that binds a third.
    ID3D11Buffer*            vb[2] = { nullptr, nullptr };
    UINT                     vb_stride[2] = { 0, 0 }, vb_offset[2] = { 0, 0 };
    ID3D11Buffer*            ib = nullptr;   // KH_MESH_INDEXED: every mesh pass binds an IB.
    DXGI_FORMAT              ib_fmt = DXGI_FORMAT_UNKNOWN;
    UINT                     ib_offset = 0;
    ID3D11VertexShader*      vs = nullptr;
    ID3D11ClassInstance*     vs_insts[8] = {};
    UINT                     vs_inst_count = 8;
    ID3D11PixelShader*       ps = nullptr;
    ID3D11ClassInstance*     ps_insts[8] = {};
    UINT                     ps_inst_count = 8;
    ID3D11GeometryShader*    gs = nullptr;
    ID3D11HullShader*        hs = nullptr;
    ID3D11DomainShader*      ds = nullptr;
    // Our passes bind b0 and b1. The capture is offset-aware through context1:
    // the engine binds its b0 pool as windows, and a plain restore would hand
    // each window back at offset zero. Widen these arrays and the offset pairs
    // in the same edit that re-arms a bind. b2-b4 are declared by every unit
    // (CBEngView*) and read as the engine left them: no pass under a
    // StateBackup ever Sets them - their writers bracket themselves:
    // kh_vmir_begin runs in the draw hook outside any backup with its own save /
    // restore pair (g_vmir_sv_b2), and the replay's draws (kh_rp_issue_mir,
    // kh_rp_replay_b) run under KhRpSave, which holds b0 - b13. No pass of ours
    // sets compute state (KH_MIR_DRAW
    // made the b2 patch a draw). A pass that Sets b2-b4 or CS state under a
    // backup widens this capture in the same edit.
    ID3D11Buffer*            vs_cbs[2] = { nullptr, nullptr };
    ID3D11Buffer*            ps_cbs[2] = { nullptr, nullptr };
    UINT                     vs_cb_first[2] = { 0, 0 }, vs_cb_num[2] = { 0, 0 };
    UINT                     ps_cb_first[2] = { 0, 0 }, ps_cb_num[2] = { 0, 0 };
    bool                     cb_offsets = false;
    // t19 is in the saved range on purpose: the effect unit binds a LUT there
    // and never nulls it. Beyond the mesh set: t20 far pyramid, t21 / t22
    // shadow pre / post, t24 volume stencil, t25-t27 hero / mid / outer sun
    // maps, t28 mirror stencil, t29-t31 their pyramids, t32 far band, t35 cast
    // occupancy, t36 DLS array, t37 dlsw mask (the DLS world pass) or the
    // near-plane marker (the effect chain, KH_NEARZ_MARK), t38 material table, t23 the
    // volume copy's depth, t40 the light ring, t42 SPECCOLOR, t43-t48 the
    // KH_USER_TEX pages (a user effect's too: KH_FX_TEX), t33 the KH_VOL_FOOT
    // mask (t39 free at the pixel
    // stage - it is the object records' VS slot, vs_srv39 below; t34 and t41
    // are the replay merges' - PSReplayMerge / PSReplayMergeMir, which bind
    // and null them, with t49-t51 past this range, under their own KhRpSave
    // (t0-t63) - and the effect chain's KH_VM_SEE snapshot / transmittance,
    // bound by kh_fx_chain_run under the flush's backup; SSAO binds t0-t3
    // under its own StateBackup).
    ID3D11ShaderResourceView* ps_srvs[49] = {};   // t0..t48.
    ID3D11ShaderResourceView* vs_srv39 = nullptr;   // KH_OBJBUF: the object record buffer's VS slot.
    ID3D11ShaderResourceView* vs_srv52 = nullptr;   // KH_SKIN_POOL: the Sk twins' chunk (kh_inst_sk_bind).
    ID3D11SamplerState*      ps_samps[2] = {};   // s0 material, s1 the shadow Gather (KH_SHADOW_GATHER).
    ID3D11DepthStencilState* dss = nullptr;
    UINT                     stencil_ref = 0;
    ID3D11BlendState*        blend = nullptr;
    FLOAT                    blend_factor[4] = {};
    UINT                     sample_mask = 0xFFFFFFFF;
    ID3D11RasterizerState*   rasterizer = nullptr;
    UINT                     scissor_n = 0;   // The engine's scissor rects, restored verbatim.
    D3D11_RECT               scissor[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    // The engine's viewports, restored like the scissors, so a pass that
    // unwinds hands them back too. One difference from the scissors: a capture
    // that found none bound restores none rather than setting zero of them, so
    // ours would survive that unwind - the engine has at least one bound at
    // every site we capture from. A pass's own viewport save stays (some hand
    // the engine's back mid-pass) and must be taken with the captured
    // viewports bound - this restore runs last and would override it.
    UINT                     vp_n = 0;
    D3D11_VIEWPORT           vp[D3D11_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE] = {};
    // A predicate set by the engine would predicate our draws too: cleared for
    // the pass, handed back with its value.
    ID3D11Predicate*         predicate = nullptr;
    BOOL                     predicate_value = FALSE;

    StateBackup() = default;
    StateBackup(const StateBackup&) = delete;
    StateBackup& operator=(const StateBackup&) = delete;
    ~StateBackup() { if (armed && ctx_saved) restore(ctx_saved); }
    void capture(ID3D11DeviceContext* ctx) {
        ctx_saved = ctx;
        armed = true;
        ctx->IAGetInputLayout(&input_layout);
        ctx->IAGetPrimitiveTopology(&topology);
        ctx->IAGetVertexBuffers(0, 2, vb, vb_stride, vb_offset);
        ctx->IAGetIndexBuffer(&ib, &ib_fmt, &ib_offset);
        ctx->VSGetShader(&vs, vs_insts, &vs_inst_count);
        ctx->PSGetShader(&ps, ps_insts, &ps_inst_count);
        ctx->GSGetShader(&gs, nullptr, nullptr);
        ctx->HSGetShader(&hs, nullptr, nullptr);
        ctx->DSGetShader(&ds, nullptr, nullptr);
        // Offset-aware when the driver offsets and the context is 1.1; the
        // plain pair otherwise, never both. A driver without offsetting drops
        // Set1, so a restore through it would leave our buffer bound
        // (g_cb_offsetting).
        ID3D11DeviceContext1* khsb_c1 = g_cb_offsetting ? kh_ctx1(ctx) : nullptr;

        if (khsb_c1) {
            khsb_c1->VSGetConstantBuffers1(0, 2, vs_cbs, vs_cb_first, vs_cb_num);
            khsb_c1->PSGetConstantBuffers1(0, 2, ps_cbs, ps_cb_first, ps_cb_num);
            cb_offsets = true;
        } else {
            ctx->VSGetConstantBuffers(0, 2, vs_cbs);
            ctx->PSGetConstantBuffers(0, 2, ps_cbs);
            cb_offsets = false;
        }
        ctx->PSGetShaderResources(0, _countof(ps_srvs), ps_srvs);
        ctx->VSGetShaderResources(39, 1, &vs_srv39);   // KH_OBJBUF.
        ctx->VSGetShaderResources(52, 1, &vs_srv52);   // KH_SKIN_POOL.
        ctx->PSGetSamplers(0, _countof(ps_samps), ps_samps);
        ctx->OMGetDepthStencilState(&dss, &stencil_ref);
        ctx->OMGetBlendState(&blend, blend_factor, &sample_mask);
        ctx->RSGetState(&rasterizer);
        scissor_n = 0;
        ctx->RSGetScissorRects(&scissor_n, nullptr);
        if (scissor_n > _countof(scissor)) scissor_n = _countof(scissor);
        if (scissor_n) ctx->RSGetScissorRects(&scissor_n, scissor);
        vp_n = _countof(vp);
        ctx->RSGetViewports(&vp_n, vp);
        if (vp_n > _countof(vp)) vp_n = _countof(vp);
        ctx->GetPredication(&predicate, &predicate_value);
        if (predicate) ctx->SetPredication(nullptr, FALSE);
    }

    void restore(ID3D11DeviceContext* ctx) {
        ctx->IASetInputLayout(input_layout);
        ctx->IASetPrimitiveTopology(topology);
        ctx->IASetVertexBuffers(0, 2, vb, vb_stride, vb_offset);
        ctx->IASetIndexBuffer(ib, ib_fmt, ib_offset);
        ctx->VSSetShader(vs, vs_inst_count ? vs_insts : nullptr, vs_inst_count);
        ctx->PSSetShader(ps, ps_inst_count ? ps_insts : nullptr, ps_inst_count);
        ctx->GSSetShader(gs, nullptr, 0);
        ctx->HSSetShader(hs, nullptr, 0);
        ctx->DSSetShader(ds, nullptr, 0);
        // Offset-aware restore, one slot at a time: a slot captured with a
        // window goes back through Set1; an empty slot, or a capture without
        // context1, takes the plain setter.
        {
            ID3D11DeviceContext1* khsb_c1 = cb_offsets ? kh_ctx1(ctx) : nullptr;

            for (UINT khsb_i = 0; khsb_i < 2; ++khsb_i) {
                if (khsb_c1 && vs_cbs[khsb_i] && vs_cb_num[khsb_i] > 0) {
                    khsb_c1->VSSetConstantBuffers1(khsb_i, 1, &vs_cbs[khsb_i], &vs_cb_first[khsb_i], &vs_cb_num[khsb_i]);
                } else {
                    ctx->VSSetConstantBuffers(khsb_i, 1, &vs_cbs[khsb_i]);
                }

                if (khsb_c1 && ps_cbs[khsb_i] && ps_cb_num[khsb_i] > 0) {
                    khsb_c1->PSSetConstantBuffers1(khsb_i, 1, &ps_cbs[khsb_i], &ps_cb_first[khsb_i], &ps_cb_num[khsb_i]);
                } else {
                    ctx->PSSetConstantBuffers(khsb_i, 1, &ps_cbs[khsb_i]);
                }
            }
        }
        ctx->PSSetShaderResources(0, _countof(ps_srvs), ps_srvs);
        kh_bc_forget();   // KH_BIND_FILTER: t0 - t48 hold the capture's views again.
        ctx->VSSetShaderResources(39, 1, &vs_srv39);   // KH_OBJBUF.
        ctx->VSSetShaderResources(52, 1, &vs_srv52);   // KH_SKIN_POOL.
        ctx->PSSetSamplers(0, _countof(ps_samps), ps_samps);
        ctx->OMSetDepthStencilState(dss, stencil_ref);
        ctx->OMSetBlendState(blend, blend_factor, sample_mask);
        ctx->RSSetState(rasterizer);
        ctx->RSSetScissorRects(scissor_n, scissor_n ? scissor : nullptr);
        if (vp_n > 0) ctx->RSSetViewports(vp_n, vp);
        if (predicate) ctx->SetPredication(predicate, predicate_value);
        armed = false;
        KH_SAFE_RELEASE(input_layout);
        KH_SAFE_RELEASE(vb[0]);
        KH_SAFE_RELEASE(vb[1]);
        KH_SAFE_RELEASE(ib);
        KH_SAFE_RELEASE(vs);
        for (UINT i = 0; i < vs_inst_count; ++i) KH_SAFE_RELEASE(vs_insts[i]);
        KH_SAFE_RELEASE(ps);
        for (UINT i = 0; i < ps_inst_count; ++i) KH_SAFE_RELEASE(ps_insts[i]);
        KH_SAFE_RELEASE(gs);
        KH_SAFE_RELEASE(hs);
        KH_SAFE_RELEASE(ds);
        KH_SAFE_RELEASE(vs_cbs[0]);
        KH_SAFE_RELEASE(vs_cbs[1]);
        KH_SAFE_RELEASE(ps_cbs[0]);
        KH_SAFE_RELEASE(ps_cbs[1]);
        for (UINT khsb_i = 0; khsb_i < _countof(ps_srvs); ++khsb_i) KH_SAFE_RELEASE(ps_srvs[khsb_i]);
        KH_SAFE_RELEASE(vs_srv39);
        KH_SAFE_RELEASE(vs_srv52);
        for (UINT khsb_j = 0; khsb_j < _countof(ps_samps); ++khsb_j) KH_SAFE_RELEASE(ps_samps[khsb_j]);
        KH_SAFE_RELEASE(dss);
        KH_SAFE_RELEASE(blend);
        KH_SAFE_RELEASE(rasterizer);
        KH_SAFE_RELEASE(predicate);
    }
};

// OMSetRenderTargets(N, ..) nulls every slot it does not name, so a get/set
// pair carrying only slot 0 would drop an engine multi-target bind. Every
// flush-side target swap goes through this: full slot set in, full slot set
// out.

// The engine binds unordered-access views on the OM too
// (OMSetRenderTargetsAndUnorderedAccessViews is hooked), and a plain
// OMSetRenderTargets would drop them: the capture reads the UAVs behind the
// bound RTVs and the restore carries them back (counters kept). With no UAV
// bound, the set is the plain call it always was. Self-restoring on unwind
// (see StateBackup); release() disarms.
struct KhOmSave {
    ID3D11RenderTargetView*    rtv[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    ID3D11DepthStencilView*    dsv = nullptr;
    ID3D11UnorderedAccessView* uav[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = {};
    UINT                       uav_start = 0;   // = the bound RTV count; UAVs occupy the slots above it.
    UINT                       uav_n = 0;       // UAVs found (0 = the plain set).
    ID3D11DeviceContext*       ctx_saved = nullptr;
    bool                       armed = false;
    KhOmSave() = default;
    KhOmSave(const KhOmSave&) = delete;
    KhOmSave& operator=(const KhOmSave&) = delete;
    ~KhOmSave() { if (armed && ctx_saved) restore(ctx_saved); release(); }
    void capture(ID3D11DeviceContext* ctx) {
        ctx_saved = ctx;
        armed = true;
        ctx->OMGetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, &dsv);
        uav_start = 0;
        uav_n = 0;
        for (UINT khom_i = 0; khom_i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++khom_i) if (rtv[khom_i]) uav_start = khom_i + 1;
        if (uav_start < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT) {
            // The Get writes from element 0 of the array it is handed, so the
            // array is offset by the start slot to keep slot i at uav[i] (set
            // reads it there).
            ctx->OMGetRenderTargetsAndUnorderedAccessViews(0, nullptr, nullptr, uav_start,
                                                           D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT - uav_start, uav + uav_start);
            for (UINT khom_i = uav_start; khom_i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++khom_i) if (uav[khom_i]) uav_n = khom_i + 1 - uav_start;
        }
    }
    void set(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* khom_dsv) {
        if (uav_n == 0) {
            ctx->OMSetRenderTargets(D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT, rtv, khom_dsv);
            return;
        }
        static const UINT khom_keep[D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT] = { ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u, ~0u };
        ctx->OMSetRenderTargetsAndUnorderedAccessViews(uav_start, rtv, khom_dsv, uav_start, uav_n, uav + uav_start, khom_keep);
    }
    // Rebind the saved targets under a different depth view: only the DSV
    // moves.
    void set_with_dsv(ID3D11DeviceContext* ctx, ID3D11DepthStencilView* khom_dsv) { set(ctx, khom_dsv); }
    void restore(ID3D11DeviceContext* ctx) { set(ctx, dsv); }
    void release() {
        armed = false;
        for (UINT khom_i = 0; khom_i < D3D11_SIMULTANEOUS_RENDER_TARGET_COUNT; ++khom_i) {
            KH_SAFE_RELEASE(rtv[khom_i]);
            KH_SAFE_RELEASE(uav[khom_i]);
        }
        KH_SAFE_RELEASE(dsv);
        uav_n = 0;
    }
};

// KH_GLOW_PYR - one glow draw's t3 (the pyramid) and s2 (the anisotropic sampler), each put back as it was by
// unbind() or the scope's end (the SSGI resolve's KH_FX_SAMP_S2 rule; StateBackup keeps s0 - s1 only).
struct KhGlowBind {
    ID3D11DeviceContext*      ctx = nullptr;
    ID3D11SamplerState*       s2 = nullptr;
    ID3D11ShaderResourceView* t3 = nullptr;
    KhGlowBind() = default;
    KhGlowBind(const KhGlowBind&) = delete;
    KhGlowBind& operator=(const KhGlowBind&) = delete;
    ~KhGlowBind() { unbind(); }
    void bind(ID3D11DeviceContext* khgn_ctx, const KhGlowPyr& khgn_p) {
        if (ctx || !khgn_ctx || !khgn_p.srv || !g_res.glow_sampler) return;
        ctx = khgn_ctx;
        ctx->PSGetSamplers(2, 1, &s2);
        ctx->PSGetShaderResources(3, 1, &t3);
        ctx->PSSetSamplers(2, 1, &g_res.glow_sampler);
        ctx->PSSetShaderResources(3, 1, &khgn_p.srv);
    }
    void unbind() {
        if (!ctx) return;
        ctx->PSSetShaderResources(3, 1, &t3);
        ctx->PSSetSamplers(2, 1, &s2);
        KH_SAFE_RELEASE(t3);
        KH_SAFE_RELEASE(s2);
        ctx = nullptr;
    }
};

// KH_ANA_PYR - one anamorphic draw's t3 / t6 (the even / odd atlases), each put back as it was by unbind() or the
// scope's end (KhGlowBind's rule). No sampler: the taps are Loads.
struct KhAnaBind {
    ID3D11DeviceContext*      ctx = nullptr;
    ID3D11ShaderResourceView* t3 = nullptr;
    ID3D11ShaderResourceView* t6 = nullptr;
    KhAnaBind() = default;
    KhAnaBind(const KhAnaBind&) = delete;
    KhAnaBind& operator=(const KhAnaBind&) = delete;
    ~KhAnaBind() { unbind(); }
    void bind(ID3D11DeviceContext* khan_ctx, const KhAnaPyr& khan_p) {
        if (ctx || !khan_ctx || !khan_p.srv[0] || !khan_p.srv[1]) return;
        ctx = khan_ctx;
        ctx->PSGetShaderResources(3, 1, &t3);
        ctx->PSGetShaderResources(6, 1, &t6);
        ctx->PSSetShaderResources(3, 1, &khan_p.srv[0]);
        ctx->PSSetShaderResources(6, 1, &khan_p.srv[1]);
    }
    void unbind() {
        if (!ctx) return;
        ctx->PSSetShaderResources(6, 1, &t6);
        ctx->PSSetShaderResources(3, 1, &t3);
        KH_SAFE_RELEASE(t6);
        KH_SAFE_RELEASE(t3);
        ctx = nullptr;
    }
};

inline void mul_4x4(const float a[4][4], const float b[4][4], float out[4][4]) {
    for (int r = 0; r < 4; ++r)
        for (int c = 0; c < 4; ++c)
            out[r][c] = a[r][0]*b[0][c] + a[r][1]*b[1][c] + a[r][2]*b[2][c] + a[r][3]*b[3][c];
}
// KH_NO_PARK_LANDED: the cycle of the last landing, stamped beside the serial.
// Render thread only: read by flush_locked's injected_since_last_flush.
static uint64_t g_composite_land_cycle = ~0ull;

inline uint64_t steady_now_ms() {
    return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
}

// The one rule for whether an object takes part in shadowing (kh_shadow_part:
// the seam footprints, which are also our pixel-ownership lists) and whether it
// casts (kh_shadow_active: the sun / DLS maps, the world cast, caster demand).
// An unlit mesh neither receives nor casts (effect meshes are unlit by
// construction). casterOnly admits an invisible object as a caster;
// KH_SHADOW_SWITCH: castShadow false makes it no caster at all, casterOnly or
// not, and so does every shadow off (KH_SHADOW_OFF): kh_cast_on. Every caster
// gather and demand census reads these - kh_cast_on where it has taken the
// part test already - never the fields directly.
// KH_INFRONT: whether the cycle that last ended drew the engine's view-model
// slice (a hands prepass or colour pass was seen). Written by the render thread
// at the main depth clear (kh_infront_frame_reset), read by every caster
// gather and demand census on either thread; atomic because the flush's
// pre-park census reads it outside the park. An inFront mesh casts only while
// this holds - in third person there is no slice, so no shadow from a mesh that
// is not drawn.
static std::atomic<bool> g_vm_slice_live{false};
// KH_SHOW_INFRONT: the verdict above is a measurement - true when the detector ran through the whole cycle that
// last ended (armed at its start, g_vm_watch_cycle). A cycle no detector watched says nothing about first person.
static std::atomic<bool> g_vm_slice_known{false};
static std::vector<float>    g_thm_data;
static UINT g_thm_w = 0;
static UINT g_thm_h = 0;
static float                 g_thm_origin[2] = {};
static float                 g_thm_cell = 0.0f;
// KH_THM_EXACT - the engine's triangle split for the staging grid, as KhThmHeight reads it (thmParams.w - 1): class
// k's split (kh_cloth_ground_prepare's g_ground_split: 0, 1, or 2 = bilinear) in base-3 digit k. A grid not at the
// matrix's native spacing, or a split not measured, is bilinear throughout (KH_THM_SPLIT_SMOOTH). The autobuild writes
// it with the grid; the upload publishes it (g_thml_split) with the grid's other live meta.
static constexpr int         KH_THM_SPLIT_SMOOTH = 2 + 2 * 3 + 2 * 9 + 2 * 27;
static int                   g_thm_split = KH_THM_SPLIT_SMOOTH;
// KH_NO_PARK_THM: handoff between the autobuild (game thread writes the staging
// grid, then sets it) and kh_thm_upload (the flush reads the grid, then clears
// it). The builder waits while it is set, so the grid is frozen until the
// upload's last read.
static std::atomic<bool>     g_thm_dirty{ false };
// Live texture usable. Written by kh_thm_upload_body (flush_locked: the render thread or a park) and by the
// device release (kh_thm_device_lost); read by the drawers on those same threads.
static std::atomic<bool>     g_thm_valid{ false };
// KH_THM_DEVRESET: a Resources::release dropped a live heightfield texture (render thread, or the game thread
// under the graphics lock). kh_thm_autobuild_step (game thread) turns it into a g_thm_dirty raise once the grid is
// complete - the flag stays the autobuild -> flush hand-off, never raised beside the autobuild's writes.
static std::atomic<bool>     g_thm_reupload{ false };
typedef void (STDMETHODCALLTYPE* FnDraw)(ID3D11DeviceContext*, UINT, UINT);
static FnDraw                    g_orig_draw = nullptr;

struct ReorderState {
    bool injected = false;   // This scene frame already got its injection.
    bool in_injection = false;   // Our own draws in flight: bypass tracking.
    bool blend_translucent = false;   // Current blend state alpha-blends.
    bool dss_nowrite = false;   // Current dss tests but does not write depth.
    bool dsv_main = false;   // The main scene depth is bound.
    uint32_t opaque_draws = 0;   // Opaque main-scene draws since the last main depth clear.
    uint32_t inject_attempts = 0;   // Injection attempts this scene frame (retry bound).
    float cycle_vp_min = 0.0f;
    float cycle_vp_max = 1.0f;
    bool  cycle_vp_valid = false;
    float trig_vp_min = 0.0f;
    float trig_vp_max = 1.0f;
    bool  trig_vp_valid = false;
    RVExtBridge::ProjectionViewTransform cycle_pv = {};
    bool  cycle_pv_valid = false;
    bool  cycle_pv_stale = false;   // This cycle's fetch failed; pv is the kept.
    float engine_m22 = 0.0f;
    float engine_m32 = 0.0f;
    bool  engine_proj_valid = false;
    // Read-only: it never touches the lock, adoption, or re-arm machinery.
    float slot_near_live = -1.0f;
    float slot_m22 = 0.0f;
    float slot_m32 = 0.0f;
    float world_m22 = 0.0f;
    float world_m32 = 0.0f;
    bool  world_pair_valid = false;
    uint64_t frame_cycles = 0;   // depth-clear cycles this frame (KH_CAST_FRAME64: 64-bit - at 16 bits it wrapped
                                 // every ~4.5 min at 240 fps, and the cast latch compares it with !=).
    uint32_t opaques_since_inject = 0;
    // Set when an anomalous (slot-silent) cycle is seen at an injection
    // attempt; consumed by the flush, which then also draws the meshes late
    // that frame - identical overdraw if the injection survived, fill-in if a
    // foreign pass erased it.
    bool anomaly_seen = false;
};
static ReorderState g_ro;
// Restores g_ro.in_injection on every exit, exception included. Its users:
// the parked flush (kh_park_run - its save/restore pairs are skipped by a
// throw; game thread, inside the park's lock scope), kh_skin_so_batch and
// kh_uvs_step (the render thread's steps - and, for kh_uvs_step, the flush's
// thread at kh_uvs_upload's end - which declare it ahead of their StateBackup
// so that it unwinds after the restore), flush_ui_frame's lock path
// (KH_PARK_INJ, as the parked flush), the seam footprint's step
// (kh_volume_seam_inject), the mirror patch's fast-path upload
// (kh_vmir_patch_b2, KH_VMIR_FAST_OWN), the mirror re-issue's two OM
// binds (kh_vmir_begin / kh_vmir_end, KH_VMIR_OM_OWN), and KH_VM_SEE's
// snapshot (kh_vmsee_snapshot, ahead of its StateBackup) and transmittance
// re-issue's OM binds (kh_vmsee_t_begin / kh_vmsee_t_end), and KH_STEP_OWN_FLAG's
// steps: the cast fire's and its two repicks (mask_cast_engine) and the
// skin-cull catch-up (kh_skin_cull_catchup).
struct KhInjectionFlagScope {
    bool khif_prev;
    KhInjectionFlagScope() : khif_prev(g_ro.in_injection) {}
    ~KhInjectionFlagScope() { g_ro.in_injection = khif_prev; }
    KhInjectionFlagScope(const KhInjectionFlagScope&) = delete;
    KhInjectionFlagScope& operator=(const KhInjectionFlagScope&) = delete;
};

// The clear adopts the bridge PV every cycle; no teleport debounce (a held
// camera puts the wrong camera into the cascade consensus). KH_LATCH_JUMP_M is
// the distance class the injection, the fire and the seam classify against.
static constexpr float    KH_LATCH_JUMP_M   = 50.0f;
static bool     g_boundary_pv_valid = false;   // The clear-time bridge fetch succeeded this cycle (it IS g_ro.cycle_pv then).
inline float latch_cam_dist_sq(const float a[3], const float b[3]) {
    const float dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
    return dx * dx + dy * dy + dz * dz;
}

static bool  g_sun_valid = false;
static float g_sun_dir_engine[3] = { 0.0f, 1.0f, 0.0f };   // Unit vector toward the light, engine
static bool g_atlas_frame_cur = false;
static bool g_atlas_frame_prev = false;

static bool  g_pub_block_valid = false;

static float g_pub_block_amb[3] = {};
static float g_pub_block_sun[3] = {};

// Per frame, per cascade cycle: latch that matrix from the upload
// (locator-cached offset, rediscovery on miss), snapshot the bound viewport at
// the cycle's next draw as the cascade's atlas tile, and hand the table to the
// composited mesh shader, which performs the engine's own shadow compare for
// our pixels.
static constexpr uint32_t KH_LIVE_MAX_CASCADES = 8;

struct LiveShadowEntry {
    float    m[12];
    float    tile[4];
    float    cam[3];   // The camera origin this entry's transform is relative to entries persist
                       // Across frames, so each carries its own.
    uint64_t stamp;   // Finalize order; the newest entry is the atlas.
    float    time = 0.0f;   // Seal wall-clock (staleness expiry; see frame_reset).
};
struct LiveShadowState {
    std::atomic<bool> wanted{ false };   // Game thread: a visible lit mesh or a shadow-active one (the census).
    // Atlas tracking (render thread).
    void*    atlas_identity = nullptr;
    ID3D11Texture2D*          atlas_tex = nullptr;
    ID3D11ShaderResourceView* atlas_srv = nullptr;
    uint32_t atlas_size = 0;
    uint32_t atlas_fmt = 0;
    float    atlas_last_seen = 0.0f;   // Last t15 consumption of the held atlas (stamped in
                                       // mask_note_draw).
    float    first_capture_time = -1.0f;   // First band capture (receive warm-up gate).
    bool     receive_warmed = false;
    bool     srv_failed = false;
    bool     phase_on_atlas = false;
    bool     pending_valid = false;
    bool     cycle_latched = false;   // One distinct entry per cascade cycle.
    uint32_t pending_bytes = 0;   // source-CB size of the pending matrix.
    bool     pending_vp_valid = false;
    float    pending_vp[4] = {};   // topLeftX, TopLeftY, Width, Height.
    float    pending_m[12] = {};
    // Decoded from the small block per commit: its second 4x3 is a pure
    // camera-yaw rotation R with translation t2 = (0.30, -1.59, 0.01) - an
    // eye-height-shaped origin offset in yaw-rotated space.
    // KH_REG_SLICE: data holds floats [KH_REG_SLICE_LO, KH_REG_SLICE_LO + KH_REG_SLICE_N) of the upload, and only
    // when floats >= KH_REG_READ_MIN - the whole of what resolve_pair_capture / band_capture read; the rest is stale.
    struct CbRecord { void* buf = nullptr; uint32_t floats = 0; uint64_t seq = 0; float data[512]; };
    CbRecord cb_reg[16];
    uint32_t cb_reg_next = 0;
    uint64_t cb_reg_seq = 0;   // Recency: which record was written last.
    // Persistent cascade table: the engine re-renders (and re-uploads the
    // transform for) a cascade only when it needs updating, so a per-frame
    // table is empty on most frames. Entries are keyed by scale (each cascade
    // has a distinct extent) and replaced in place. The engine's cascade CB
    // exceeds the registry's size gate, so the pass-scoped stash of the last
    // window-shaped matrix is what holds it.
    LiveShadowEntry entries[KH_LIVE_MAX_CASCADES] = {};
    uint32_t count = 0;
    bool     wipe_deferred = false;   // The table was marked suspect; the first commit prunes it.
    uint64_t stamp_counter = 0;
    int      newest = -1;   // Entry index of the last finalize.
    struct BandSlot {
        bool  valid = false;
        bool  pending_view = false;
        // The provisional vcol came from the bridge fallback (no engine view
        // published yet): a cross-convention pairing the receive term must not
        // consume. Cleared when the true same-frame view completes the seal.
        bool  vcol_bridge = false;
        float pending_since = 0.0f;   // When the seal went pending.
        float sm[12] = {};   // PSC_ShadowmapMatrix rows (float4x3).
        float vcol[12] = {};   // View matrix columns 0.2 (world->view, row-vector conv).
        float border[4] = {};   // x = near, y = far.
        float last_time = 0.0f;
        ID3D11Texture2D*          tex = nullptr;
        ID3D11ShaderResourceView* srv = nullptr;
        // Deferred capture: the sm/border a sweep produced, held out of the
        // live triple until the view that pairs with it arrives. The slot keeps
        // serving its previous complete seal meanwhile.
        bool     stage_valid = false;
        float    stage_sm[12] = {};
        float    stage_border[4] = {};
        float    stage_since = 0.0f;
        float    stage_view[12] = {};
        bool     stage_view_ok = false;
        // vcol layout (columns), like stage_view above.
        bool     stage_pair_ok = false;
        // The first exact native publish accepted after this band's own stage,
        // no source filter and no boundary guard.
        float    stage_first[12] = {};
        bool     stage_first_ok = false;
        int      stage_pool = -1;   // Claimed back buffer (-1 = none).
    };
    BandSlot band[8];   // All 8 engine cascades: full-range receive.
    uint64_t band_captures = 0;
    bool     stage_any = false;   // Cheap per-draw gate for the commit.
    uint64_t band_bail_slot = 0;   // Both slots hold nearer bands.
    bool resolve_seen_since_cast = false;
    bool view_published_this_frame = false;   // Dual gate for the cast arm.
    float last_publish_rot_err = 1.0f;   // rot agreement of the last accepted publish.
    // A healthy lock re-proves exactness within the window continuously; a
    // genuinely off-axis era never does and starves - at most one window late.
    uint64_t pub_exact_ms = 0;   // 0 = no exact publication yet.
    uint64_t pub_fresh_ms = 0;
    float band_last_reject[4] = {};   // Last border quad rejected by sanity.
    uint32_t resolve_window = 0;   // Qualifying draws remaining after an atlas-pass exit.
    float frame_view[16] = {};
    bool  frame_view_valid = false;
    float frame_view_time = -1.0f;
    // Latched at the accept whenever the incoming publish crosses sources.
    float alt_view[16] = {};
    bool  alt_view_valid = false;
    float alt_view_time = -1.0f;
    struct ViewCand { ID3D11Resource* res; uint32_t off; float m[16]; };
    ViewCand vc_ring[16];
    uint32_t vc_n = 0;
    ID3D11Resource* view_src_res = nullptr;   // Learned location (weak key).
    uint32_t view_src_off = 0;
    int  view_src_orient = 0;   // 1 direct, 2 transposed.
    bool view_src_relative = false;   // Engine view is camera-relative (t ~ 0).
    bool view_src_valid = false;
    uint32_t pub_rej_streak = 0;   // Consecutive bridge-truth publish rejections.
    bool pub_ok_since_lock = false;   // This lock has passed at least one.
    uint32_t view_src_miss = 0;
    uint64_t view_locks = 0;
    // Exactness-preference wait: time of the first bar-passing but non-exact
    // candidate deferral (0 = not waiting).
    float view_wait_since = 0.0f;
    float prev_view[16] = {};   // Previous accepted view (rotation extrapolation).
    bool  prev_view_valid = false;
    bool     cache_valid = false;
    uint32_t cache_bytes = 0;
    uint32_t cache_offset = 0;
    float    cam[3] = {};   // The frame's render camera (engine axes) recorded at injection - the
                            // Shadow space origin.
};
static LiveShadowState g_ls;

// Flush serial (the scene flush: the render thread's, or the game thread's
// under a park): once-per-flush key for the miss latch.
static std::atomic<uint64_t> g_cc_flush_serial{0};   // Ticks once per scene flush (a chain-only run does not).

static float g_shadow_map_strength = 1.0f;   // 0 disables the map term.
static float g_shadow_map_bias = 0.0015f;   // In the transform's depth units.
static float g_shadow_map_sign = 1.0f;   // +1 standard depth, -1 reversed.
// Private sun-depth map state (written by the render thread's pass, read by the
// flush). vp/bias/bounds mean something only while valid; both flags reset at
// the main depth clear.
static bool  g_sun_map_rendered_frame = false;   // This frame's pass already ran.
static bool  g_sun_map_valid = false;   // Pass produced a usable map.
static float g_sun_map_render_time = -1.0f;   // Not reset by sub-settle sun wobble.
static uint64_t g_sun_mat_chash = 0;   // Caster hash at the last maturity reset.
static float    g_sun_mat_sun[3] = {};   // Derived sun at the last maturity reset.
// When it was produced (the fire consumes last frame's map; 0.25 s staleness
// bounds fallback frames).
static float g_sun_map_time = -1.0f;
static bool  g_sun_map_no_local = false;   // Casters exist, none within the fit.
static float g_sun_anchor_now[3] = {};
// KH_VOL_MIRROR - the three PODs the mesh fill reads (mirMeta).
static uint32_t g_vmir_w = 0;
static uint32_t g_vmir_h = 0;   // Mask dims (engine vol DSV size).
static float    g_vmir_mask_time = -1.0f;   // Effect time of the last prepass.
static float g_sun_map_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the union's cast bias (0 = fall
static float    g_sun2_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the cast chain's own (0 = fall
static bool     g_sun2_map_valid = false;
static float    g_sun3_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the cast chain's own (0 = fall
static bool     g_sun3_map_valid = false;
static float    g_sun4_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the cast chain's own (0 = fall
static bool     g_sun4_map_valid = false;
static float    g_sun5_cast_bias = 0.0f;   // KH_CAST_BIAS_CAP: the cast chain's own (0 = fall
static bool     g_sun5_map_valid = false;
static bool g_sun_pf_valid[4] = { false, false, false, false };   // KH_FAR_PF: [3] the far tier.
// khsh_out_reach / khsh_out_casters are written only inside the
// completed-render block, so with valid false they are stale from the last
// completing frame. The lanes in rendering_integration_features.hpp are stamped at the admission loop, which
// every call runs before any exit.
static constexpr float KH_BAND_BIAS_TEXELS = 2.0f;
// -1 = the cast fill never ran.
static constexpr float KH_BAND_FWD_SLACK = 1.25f;
static constexpr float KH_BAND_FWD_DROP  = 1.60f;   // re-latch tighter past this.
static uint64_t g_sun_map_hash = 0;
static float g_sun_map_bounds[6] = {};   // Combined caster AABB: center xyz, half extents xyz

// In-map casters, 6 floats each: center xyz + half extents xyz (engine axes) -
// uncapped.
static std::vector<float> g_sun_locals;
static int   g_sun_local_count = 0;   // Valid entries above (render thread, like the rest).
// Bumped wherever g_sun_locals is rewritten: the occupancy memo's key is an FNV
// over the whole array, so with a serial the hash runs once per rewrite instead
// of once per fire.
static uint64_t g_sun_locals_serial = 0;

// Provenance (disassembly of the lit material shaders): the engine's non-sun
// lighting is per-pixel, driven by PS cb10[4] (control: [0].x point count and
// [1].x spot count as raw INT bit patterns, [2].x global distance scale,
// [3].xyz global diffuse multiplier) and cb11[192] (32 lights x 6 float4,
// points first, spots appended; record layout in the HLSL cbuffer comment).

static constexpr uint32_t KH_DL_MAX_LIGHTS = 32;   // cb11 capacity (192 / 6).
// That bound is the ENGINE's: one of its per-draw lists carries at most 32. The
// merged pool is uncapped, and so is what one of our meshes receives from it
// (KH_DL_RING, kh_dl_select).
static constexpr uint32_t KH_DL_LIGHT_BYTES = 6 * 16;
static constexpr uint32_t KH_DL_RING = 16;
static constexpr uint32_t KH_DL_WIN_MAX = 16;   // Distinct per-draw windows captured per frame
static constexpr uint32_t KH_DL_VS_SLOTS = 13;   // VS cb slots captured (0.12).
// The bar a candidate must clear to author. The invariant that a real origin
// does not wander is in the ranking (khd_offer), so the bar admits everything
// the arbiter allows.
static constexpr uint32_t KH_DL_ANCHOR_N = 64;
static constexpr uint64_t KH_DL_POOL_STALE_MS = 500;
// A harvest gap past this is a stall (alt-tab, load, time skip), not a frame.
// KH_DL_WIN_SLOTS: wider than KH_DL_WIN_MAX because this table remembers
// windows across harvests as the engine cycles buffer addresses.
static constexpr uint32_t KH_DL_WIN_SLOTS = KH_DL_WIN_MAX * 4u;
// KH_DL_PRES_CUR: the distinct origins one harvest's placed windows with a live capture camera remember (more are not
// kept).
static constexpr uint32_t KH_DL_CUR_N = 4;

// The engine's constant buffers are shadowed on the CPU at the Unmap /
// UpdateSubresource hooks; the light sampler reads each draw's cb10 / cb11 / VS
// windows from those shadows. The harvest at the injection merges what was
// drawn since the last injection (the span), keyed on the shadow's write
// generation, so every list the engine uploaded is seen (cb11 is DISCARD-mapped
// for nearly every draw). That completeness is what lets presence replace the
// TTL.
static constexpr uint32_t KH_DL_WIN_MAX_CPU = 64;   // Windows per span (authoring + routine).
// A record the harvest could not place (no origin by derivation or vote) is
// remembered by its raw list-frame position, quantized to a metre, for
// KH_DL_UNPL_KEEP harvests; an upload whose unknown records are all remembered
// earns no window past the routine quota until that lapses, a buffer with no
// origin included (KH_DL_UNPL_ORIGIN) - unless it also carries two known
// lights, which clear the vote's bar: its noted records then author as
// unknown ones do. Presence and placeable lights are untouched.
static constexpr uint32_t KH_DL_UNPL_N = 256;     // Table entries (open addressing, 8-slot probe).

struct DlWindowMeta {
    void*    buf = nullptr;   // Weak identity (dedupe key with first).
    uint32_t first = 0;
    uint32_t bytes11 = 0;   // Lights bytes actually copied.
    uint16_t vs_bytes[KH_DL_VS_SLOTS] = {};   // VS constants copied per slot (0 = absent).
    // The cb11 shadow's write generation (0 = a GPU-arena window), the content
    // hash of ctl + lights (per-harvest dedupe) and the harvest this window
    // belongs to.
    uint32_t gen = 0;
    uint64_t chash = 0;
    uint64_t seq = 0;
    uint64_t wseq = 0;   // KH_DL_REC_ORDER: the cb11 content's capture order (its shadow's wseq; 0 = GPU arena).
    // KH_DL_WIN_VIEW: the frame view (3 float4 columns) and true-world camera this
    // window was drawn under (kh_dl_meta_view). A trigger-to-trigger span holds the
    // previous frame's post-trigger draws and this frame's, which need not share a
    // camera, so one latch per span cannot serve both.
    float    view[12] = {};
    float    cam[3] = {};
    uint8_t  view_ok = 0;
    uint8_t  view_live = 0;   // KH_DL_PRES_CUR: the view is the clear's own fetch (kh_dl_view_live).
};

struct DlPoolLight {
    float    rec[24] = {};   // The 6-float4 record verbatim (producer side) +0 rewritten to
                             // Absolute world.
    uint8_t  spot = 0;   // Index >= its list's pointN.
    uint64_t stamp = 0;   // Last re-sight (persistence TTL).
    uint16_t gap_max_ms = 0;   // Worst observed re-sight gap (cadence evidence).
    uint16_t sightings = 0;
    // Session-unique identity, minted at first sight, carried through
    // compaction. 0 = unminted. Lets a shadow slot name its light when two
    // lights tie on position (a flashlight and a muzzle flash on one weapon).
    uint32_t id = 0;
    // The newest harvest whose span carried this light; presence at harvest N
    // is seq == N. The depth-clear cycle is not usable as the key: the engine
    // clears the main depth many times a frame at night.
    uint64_t seq = 0;
    uint64_t first_seq = 0;   // KH_DL_DIAG (CN-4): the harvest (g_dl.harvest_seq) whose merge created it.
    // KH_DL_REC_ORDER: the capture order (KhCbShadow::wseq) of the content rec holds; 0 = unordered (a GPU-arena
    // window's). A write never replaces a newer content with an older one.
    uint64_t rec_wseq = 0;
};

struct DlRingEntry {
    uint32_t point_n = 0, spot_n = 0;
    float    cam[3] = {};   // Capture camera (engine axes).
    float    l0[8] = {};   // First light: pos.xyz, diffuse.rgb, fadeStart, coneCos.
};

struct DynLightsState {
    // Published mirror (render thread writes at harvest; getRenderStats reads
    // it on the game thread - dlValid and the light list - and the DL ring
    // entry takes cam).
    bool     valid = false;
    uint32_t point_n = 0, spot_n = 0;
    float    ctl[16] = {};
    float    lights[KH_DL_MAX_LIGHTS * 24] = {};
    float    view_cols[12] = {};   // 3 float4 view columns.
    bool     view_valid = false;
    float    cam[3] = {};   // Capture camera (engine axes).
    uint64_t stamp_ms = 0;
    uint64_t hash = 0;
    // Staging machinery (render thread only).
    ID3D11Buffer* staging[2] = {};
    uint8_t  inflight[2] = {};
    uint32_t slot_bytes11[2] = {};   // Lights bytes actually copied.
    float    pending_view[2][12] = {};
    float    pending_cam[2][3] = {};
    uint8_t  pending_view_ok[2] = {};
    ID3D11Buffer* arena[2] = {};
    uint8_t  arena_inflight[2] = {};
    int      arena_write = 0;
    DlWindowMeta win_meta[2][KH_DL_WIN_MAX_CPU] = {};   // KH_DL_CPU: the CPU cap bounds both.
    uint32_t win_count[2] = {};
    std::vector<uint8_t> cpu_arena[2];   // KH_DL_CPU: the same window layout, CPU-resident.
    uint8_t  side_miss[2] = {};          // KH_DL_CPU: a draw this side could not capture whole.
    uint32_t side_uploads[2] = {};       // KH_DL_PRESENCE: uploads seen this side.
    uint64_t harvest_seq = 0;            // KH_DL_SEQ: harvests so far; uploads are tagged with the next.
    int32_t  unpl_q[KH_DL_UNPL_N][3] = {};   // KH_DL_UNPL: quantized list-frame positions...
    uint64_t unpl_seq[KH_DL_UNPL_N] = {};    // ...and the harvest that noted them (0 = empty).
    // Merged absolute-world pool (render thread writes at harvest; the draw
    // paths read).
    std::vector<DlPoolLight> pool;   // Uncapped; grows at harvest, TTL-swept.
    uint32_t pool_n = 0;
    // Anchor table (derived-provenance light positions; the vote's ground
    // truth, decoupled from shading persistence).
    float    anchor_pos[KH_DL_ANCHOR_N][3] = {};
    uint64_t anchor_stamp[KH_DL_ANCHOR_N] = {};
    uint32_t anchor_n = 0;
    // Derivation-candidate recon ring (every validate-pass from the full
    // scans).
    uint64_t pool_stamp = 0;
    float    main_gdiff[3] = { 1.0f, 1.0f, 1.0f };
    float    main_scale = 1.0f;
    uint8_t  main_ref_valid = 0;
    // KH_DL_REF_MEDIAN: the brightest candidate's (main_gdiff rgb, main_scale) of the last three harvests that had
    // one (a placed window, or KH_DL_REF_SEEN an upload re-sighted through its buffer's origin), oldest first, and
    // how many are held (0 - 3; the session reset empties it).
    float    ref_hist[3][4] = {};
    uint32_t ref_hist_n = 0;
    // Per-window resolved origin.
    float    win_ox[KH_DL_WIN_SLOTS] = {};
    float    win_oz[KH_DL_WIN_SLOTS] = {};
    uint8_t  win_oset[KH_DL_WIN_SLOTS] = {};
    uint64_t win_oseq[KH_DL_WIN_SLOTS] = {};   // KH_DL_PRES_FRESH: the harvest that set it (0 = none).
    // KH_DL_PRES_CUR: the origins (x, z) harvest cur_oseq placed windows with a live capture camera (view_live) at
    // (0 = none yet), cur_n of them, each with the capture camera (x, z) of the last such window placed there; and
    // whether it also placed one without a live camera, or at a (KH_DL_CUR_N + 1)th origin (cur_amb).
    float    cur_o[KH_DL_CUR_N][4] = {};
    uint32_t cur_n = 0;
    uint64_t cur_oseq = 0;
    uint8_t  cur_amb = 0;   // Harvest cur_oseq placed a window that could contradict unseen: it vouches for none.
    // KH_DL_REF_SEEN: span ref_seen_seq's brightest ctl (gdiff rgb, scale) among the uploads presence re-sighted a
    // record of through their buffer's origin, and its brightness (the sum of the three; -1 = none).
    float    ref_seen[4] = {};
    float    ref_seen_lum = -1.0f;
    uint64_t ref_seen_seq = 0;
    // The pool vote runs as a confirmation, not only a fallback: the engine
    // culls its light list to the camera, so a correct origin places at least
    // one of a window's lights near the camera and a wrong one displaces them
    // all. Keyed per window: a shared tuple would let the last window to finish
    // overwrite another's.
    uint8_t  win_hint_set[KH_DL_WIN_SLOTS] = {};
    uint8_t  win_hint_kind[KH_DL_WIN_SLOTS] = {};
    int32_t  win_hint_slot[KH_DL_WIN_SLOTS] = {};
    uint32_t win_hint_off[KH_DL_WIN_SLOTS] = {};
    uint8_t  win_hint_form[KH_DL_WIN_SLOTS] = {};
    int32_t  win_hint_wslot[KH_DL_WIN_SLOTS] = {};
    uint32_t win_hint_woff[KH_DL_WIN_SLOTS] = {};
    uint8_t  win_hint_wform[KH_DL_WIN_SLOTS] = {};
    uint32_t win_fail_lo[KH_DL_WIN_SLOTS] = {};
    uint32_t win_fail_run[KH_DL_WIN_SLOTS] = {};
    uint64_t win_use[KH_DL_WIN_SLOTS] = {};   // KH_DL_SLOT_LRU: the lookup that last named the entry.
    uint64_t win_use_n = 0;                   // Lookups so far (kh_dl_win_slot).
    DlRingEntry ring[KH_DL_RING];
    uint32_t ring_head = 0;
};
static DynLightsState g_dl;
static uint32_t  g_dls_n = 0;
// KH_DLS_ALLOW: allowDynamicShadows (script-set; off by default and at every
// session start). Off: kh_dls_select holds no slot, so no light renders a
// map, the world pass stands down and every mesh reads its dynamic-light
// shadows as lit. The lights still light the meshes (fill_dynlights_cb).
static std::atomic<bool> g_dls_allowed{ false };
static std::atomic<uint32_t> g_ao_strength_bits{ 0x3F800000u };   // Float bits, default 1.0; 0 = off.
static std::atomic<uint32_t> g_ao_dist_bits{ 0x3F000000u };       // The sample radius (m), default 0.5.
// The pass's parameters, filled by kh_ssao_pre and read by kh_ssao_post in
// the same injection (a stack object of inject_composited_meshes).
struct KhSsaoPass {
    bool        on = false;      // The resources stand; the post pass may run.
    D3D11_RECT  rect = {};       // The pixel rectangle both passes draw over.
    float       proj[4] = {};    // m00, m11, m22, m32.
    float       vp_lo = 0.0f, vp_hi = 1.0f;
    UINT        w = 0, h = 0;    // The main depth's dimensions.
    // KH_SSAO_NEARZ: the drawer's near-z route - its near (0 = the pass routes nothing) and the gap floor its routed
    // draws ramp the fragments nearer than that near from, up to vp_lo. Set by the drawer before kh_ssao_post.
    float       nz_near = 0.0f, nz_gap = 0.0f;
    bool        term = false;    // KH_NEARZ_MARK: the SSAO term runs (off: the mark serves the marker alone).
    bool        nzm = false;     // KH_NEARZ_MARK: the marker has a reader this cycle (kh_nzm_wanted).
};
// The pass rectangle for a drawer: kh_snapshot_rect at its pv, padded by the
// blurs' reach; the whole target when a corner is behind the camera.
// Defined with the snapshot in rendering_integration_injection.hpp.
inline bool kh_snapshot_rect(const RVExtBridge::ProjectionViewTransform& pv, UINT khsr_w, UINT khsr_h, D3D11_RECT& khsr_out);
inline void kh_inst_sk_bind(ID3D11DeviceContext* ctx, const KhSkinPoolChunk* khsb_c,
                            ID3D11ShaderResourceView*& khsb_bound);   // Defined in rendering_integration_features.hpp.
// A run loop's vertex route: the pass's own VS and layout (vs / il, bound
// before the loop) for a run of one buffer, the Sk twin, layout_inst_sk and
// the chunk at VS t52 for a chunk's run. end() hands the loop's successor the
// pass's own pair, as the loop always left it. StateBackup holds t52.
struct KhDepthSk {
    ID3D11VertexShader* vs;
    ID3D11InputLayout*  il;
    ID3D11VertexShader* sk;
    const KhSkinPoolChunk* cur;
    ID3D11ShaderResourceView* srv;
    void use(ID3D11DeviceContext* khds_ctx, const KhSkinPoolChunk* khds_c) {
        if (khds_c && !cur) {
            khds_ctx->VSSetShader(sk, nullptr, 0);
            khds_ctx->IASetInputLayout(g_res.layout_inst_sk);
        } else if (!khds_c && cur) {
            khds_ctx->VSSetShader(vs, nullptr, 0);
            khds_ctx->IASetInputLayout(il);
        }
        kh_inst_sk_bind(khds_ctx, khds_c, srv);
        cur = khds_c;
    }
    void end(ID3D11DeviceContext* khds_ctx) { use(khds_ctx, nullptr); }
};
static uint64_t g_dls_map_valid = 0;                // Bitmask of slots with a usable map.

// Uncapped; cleared, never freed.
struct KhDlswCaster {
    float pos[3]; float size[3]; float rot[9]; bool rotated; int mesh; bool lod_lock;
    float lod_r;    // KH_SKEL_LOD_R, from the SunCaster: the mask's pick radius.
    float res[3];   // KH_POS_RES, from the SunCaster.
    uint32_t slot = 0xFFFFFFFFu;   // KH_CLOTH: which substitute vertex buffer this caster draws from.
    // KH_DLSW_MASK_ALPHA: the mask's own lanes, copied from the SunCaster.
    bool visible;                 // false = casterOnly invisible: no pixel is ours, not drawn.
    float alpha;                  // < 0.999 = a whole translucent object: writes no depth, not drawn.
    const KhMaterialSet* mats;    // Alpha-carrying set, or nullptr.
    bool alpha_caster;            // The sun's verdict (kh_sun_caster_of).
    bool in_front;                // KH_INFRONT: masked under the hands pair, after the world casters.
    bool no_foot;                 // KH_FOOT_BLEND: a see-through blend mode owns no pixel, not drawn.
};
static std::vector<KhDlswCaster> g_dlsw_casters;

// The light maps render on their own bracket, free of the sun pass's gates.
// Once per cycle, whichever mesh pass arrives first (the injection, or the
// flush fallback) - the kh_dls_select rule.
static uint64_t g_dls_frame_cycle = ~0ull;      // once-per-cycle guard (state, not a lane).
static float    g_khlt_lp[3] = {};
static bool     g_khlt_lp_ok = false;

// KH_LOCAL_CULL - a localized pass whose volume no reconstructed point can reach is skipped whole. The
// localization mask (cb.hlsl KhFxFinish) is 0 wherever the point KhWorldPosRelFenced rebuilds from the depth
// lies outside the volume and its falloff - a box of half extents max(r, 0.01) x (1 + max(falloff lane,
// 0.001)) about the centre, the ellipsoid inside it - and every such point lies inside the four side planes of
// the matrix the pass's inverse inverts: a pixel centre is inside the viewport and its depth (>= 1 m, the
// encode pair the matrix itself carries) in front of the camera. Outside one side plane, then, the mask is 0 at
// every pixel and the pass leaves the picture as it was (KH_LOCAL_BLEND: under every blend), so it is skipped
// as a zero-opacity pass is, with that skip's two side effects: the passes either side of it may fuse (one
// rounding to the scene's 16F format fewer) and, every pass skipped, the target keeps the engine's alpha.
// Not an inverse volume (its mask is 1 there), not a .hlsl effect (it applies the mask only if it calls
// KhFxFinish). Planes in double; no margin: a pixel centre lies half a pixel inside the view's edge, some
// thousand times the float reconstruction's error (hx_dd/h4). The scene chain (the flush's camera-relative
// inverse, while the passes linearise with the pair it was built with) and the PIP's localized chain (its
// own view, pair and camera).
struct KhLocalCull {
    bool   on = false;
    bool   pair_check = false;   // The scene chain: g_ro's engine pair must still be the planes' (pair).
    float  pair[2] = { 0.0f, 0.0f };
    double plane[4][4] = {};      // Inward unit normals and offsets, camera-relative.
    double cam[3] = { 0.0, 0.0, 0.0 };   // fxCam: the shader takes the centre relative to it.
};
static uint64_t g_skin_cull_pip_cycle = ~0ull;   // The cycle a PIP last reached its draws (render thread).

// Engine-side (injection) functions the feature header calls ahead of their definitions in
// rendering_integration_injection.hpp, which follows rendering_integration_features.hpp below.
static void __stdcall on_engine_reset();
inline void kh_skel_centre_d(const RenderObject& khsd_o, double khsd_pos[3]);
inline void kh_attach_skel_place(RenderObject& khsp_o, const double khsp_root[3], bool& khsp_moved);
inline void kh_attach_moved_note(const RenderObject& khmn_o, bool khmn_moved, bool khmn_rt);
inline std::string kh_scene_capture_timed(ID3D11Device* dev, ID3D11DeviceContext* ctx);
inline std::string ensure_depth_srv(ID3D11Device* dev, ID3D11DeviceContext* ctx,
                                    UINT* out_width, UINT* out_height);
inline void kh_fx_depth_pair_guard(ConstantData& cbd, uint64_t khfx_serial);
inline bool is_composite_eligible(const RenderObject& o);
inline bool kh_sky_static();
inline const float* kh_shadow_sun();
inline bool kh_sun_axis_witnessed();
inline bool kh_adopt_frame_view(RVExtBridge::ProjectionViewTransform& pv);
inline bool kh_vmsee_valid();

}

// The renderer in two halves, included here between the shared head above and the session lifecycle
// below: the feature header (our renderer: meshes, materials, physics, shaders, effects, lighting and
// shadows) first, then the injection header (the bridge to the engine: the D3D hooks, the scanners of the
// engine's uploads and memory, the injection sites and the drivers), which builds on both.
#include "rendering_integration_features.hpp"
#include "rendering_integration_injection.hpp"

namespace RenderIntegration {

// The stats reset touches statistics only - the counters and the shader-cache
// hit / miss pair - and disarms collection. Called from the game thread by
// resetRenderStats (under the graphics lock when its best-effort park gets it,
// without it when not), by rendering_integration_reset's lock-failure arm
// (unparked, the same case) and by the parked session reset, so nothing the render
// thread depends on may be written here; per-session live state is
// kh_session_scratch_reset below, which only the parked session reset reaches.
inline void reset_stat_counters() {
    g_stats_armed.store(false, std::memory_order_relaxed);
    g_stats = RenderStats{};
    g_attach_repaired.store(0, std::memory_order_relaxed);   // KH_ATTACH_DIAG.
    g_rp_c = KhRpCounters();   // KH_VOL_REPLAY.
    g_gid_breaks = 0;   // KH_GTS_IDENTITY.
    g_rtp_c = KhRtpCounters();   // KH_RT_PAINT_READ.
    g_attach_refused.store(0, std::memory_order_relaxed);
    g_attach_skew_bits.store(0, std::memory_order_relaxed);
    g_shader_cache_hits.store(0, std::memory_order_relaxed);
    g_shader_cache_misses.store(0, std::memory_order_relaxed);
    kh_prof_reset_pub();   // KH_PROF.
    g_prof_epoch.fetch_add(1u, std::memory_order_release);   // KH_PROF_DISARM: nothing measured before this is read.
    g_skin_bound_worst_bits.store(0, std::memory_order_relaxed);   // KH_SKIN_GPU: skinBoundWorst.
    g_skin_so_n.store(0, std::memory_order_relaxed);   // skinGpu / skinCpu: republished by the next cycle's skinning.
    g_skin_cpu_n.store(0, std::memory_order_relaxed);
    g_skin_cull_n.store(0, std::memory_order_relaxed);   // KH_SKIN_CULL: skinCulled.
    g_cloth_skipped.store(0, std::memory_order_relaxed);   // clothSkipped / chainSkipped / skinSkipped.
    g_chain_skipped.store(0, std::memory_order_relaxed);
    g_skin_skipped.store(0, std::memory_order_relaxed);
    g_skin_pool_full.store(0, std::memory_order_relaxed);   // skinPoolFull.
}

// Per-session scratch that is not a device object; the replay, paint-read and
// identity counters it zeroes with the state they count are statistics
// reset_stat_counters zeroes too. Must run with the render thread parked.
inline void kh_session_scratch_reset() {
    g_castocc_set.clear(); g_castocc_slack = 0.0f;
    g_castocc_pre.active = false; g_castocc_pre_noreach = false;   // KH_CASTOCC_PRE.
    g_cam_step_m = -1.0f;
    g_fire_count_frame = 0;
    g_fire_clamp_painted = 0;
    g_svs_post_made = 0;
    g_sun_band_reach[0] = g_sun_band_reach[1] = g_sun_band_reach[2] = -1.0f;
    g_sun_band_reach[3] = -1.0f;   // Far band.
    g_sun5_renders = 0; g_sun5_casters = 0;   // (validity clears at the pass gates).
    g_topo_cycles = 0;
    g_topo_cycles_pub.store(0, std::memory_order_relaxed);
    // A cycle-stamped latch restarts with the counter above: a stamp left by
    // the previous session matches once, when the new session's counter reaches
    // it, and that cycle's once-per-cycle work is skipped.
    g_mat_anim_done = ~0ull;   // KH_UV_ANIM: the rewrite's and the time's stamps (the deferred edge keeps the list).
    g_mat_anim_tcyc = ~0ull;
    // KH_ATTACH_GT_SNAP: g_gts_seq and its publication are reset with the objects (kh_session_objects_reset).
    kh_rp_release_records();   // KH_VOL_REPLAY (the targets and copies go with the device).
    kh_rp_img_clear();   // KH_REPLAY_CPUCB.
    g_rp_pass = ~0ull;
    g_rp_ok = g_rp_ended = g_rp_replayed = false;
    g_rp_merge_ok = false;
    g_rp_mir_ok = false;   // KH_MIR_REPLAY.
    g_rp_mir_want = false;
    g_rp_mirb_drawn = false;
    g_rp_mir_pend = false;
    g_rp_mir_pend_cyc = g_rp_mir_pend_pass = ~0ull;
    g_rp_sc_all = KhRpRect();   // KH_REPLAY_SCISSOR.
    g_rp_sc_mir = KhRpRect();
    g_rp_sc_gen = ~0ull;
    g_rp_pass_vol = g_rp_pass_mir = false;
    g_rp_rast_on = false;   // KH_REPLAY_RASTER.
    g_vmir_want_cycle = ~0ull;   // KH_MIR_GATE: a cycle stamp.
    g_nzm_cycle = ~0ull;   // KH_NEARZ_MARK: likewise.
    g_vmsee_want_cycle = ~0ull;   // KH_VM_SEE: likewise.
    g_vmsee_snap_cycle = ~0ull;
    g_vmsee_ok_cycle.store(~0ull, std::memory_order_relaxed);
    g_vmsee_pending = false;
    g_rp_withheld_seq = ~0ull;
    g_rp_front_seq = ~0ull;
    g_rp_eng_seq = ~0ull;
    g_rp_vol_clear_seq = ~0ull;
    g_rp_seam_b = false;
    g_rp_c = KhRpCounters();
    g_gid_ok = false;   // KH_GTS_IDENTITY: relearned in the new session.
    g_gid_k = 0;
    g_gid_breaks = 0;
    g_rtp_cyc = ~0ull;   // KH_RT_PAINT_READ.
    g_rtp_read_ser = 0;   // KH_RT_READ_SER.
    g_rtp_c = KhRtpCounters();
    g_gts_clear_qpc = 0;
    g_gts_clear_prev = 0;
    g_gts_half = 0;
    g_gts_final_cyc = ~0ull;
    g_gts_learn_cyc = ~0ull;
    g_gts_first_rd_cyc = ~0ull;
    g_gts_crew_n.store(0, std::memory_order_relaxed);   // KH_CREW_LAG.
    g_gts_first_cyc = ~0ull;      // KH_CAST_REPICK.
    g_gts_first_done = 0;
    g_attach_repick_step = false;
    g_attach_repick_moved = false;
    g_sun_restep_cycle = ~0ull;
    g_rtshadow_cycle.store(~0ull, std::memory_order_relaxed);
    g_dls_frame_cycle = ~0ull;
    g_dls_world_cycle = 0;
    g_fire_post_scene_cycle = ~0ull;
    g_ring_best_cycle = 0;
    g_ring_best_valid = false;
    g_view_relock_wide_cycle = 0;
    g_inj_dp_valid = false;
    g_inj_dp[0] = g_inj_dp[1] = g_inj_dp[2] = g_inj_dp[3] = 0.0f;
    for (int khbt_r = 0; khbt_r < 8; ++khbt_r) g_band_tab_far[khbt_r] = -1.0f;
    g_inj_enc_m22 = 0.0f; g_inj_enc_m32 = 0.0f; g_inj_enc_near = -1.0f; g_inj_enc_ms = 0;
    g_khpe_p1 = -1.0f; g_khpe_p2 = -1.0f; g_khpe_m22 = 0.0f;
    g_khpe_ms = 0; g_khpe_upd_ms = 0; g_khpe_skip1 = -1.0f;
    g_khbl_stable = -1.0f; g_khbl_prev = -1.0f;
    g_khlt_prev_ok = false; g_khlt_prev_step = 0.0f;
    g_khlt_step_ring[0] = g_khlt_step_ring[1] = 0.0f;
    g_khlt_step_ring[2] = g_khlt_step_ring[3] = 0.0f;
    g_khlt_step_ring_i = 0;
    g_khlt_lp_ok = false;
    g_khlt_ref_ok = false;
    g_ls.band_captures = 0;
    g_ls.band_bail_slot = 0;
}

// KH_SESSION_RESET - every namespace-scope global of the renderer's three headers back to its
// declared initial value, i.e. to what a fresh process holds: scalars and
// arrays to their initialisers, atomics stored, containers and structs
// rebuilt in place (kh_reinit, which also frees their memory). Generated from
// the declarations, one line each in the undivided header's declaration order; a global added to any
// of them gets a line here, or in kh_session_objects_reset below when the
// objects of a live mission hold what it names. Not reset, by design:
// synchronisation objects and thread records; the hook trampolines and install
// records (the code patches stay in place for the process, pass-through while
// disarmed); the shader pool (g_khsm_* / g_khsa_*: a detached worker may still
// read them - its own shutdown owns them); the Draw3D / UI event-handler
// handles and the draw list (their own teardown, first); the attachment
// table and the proxy pool with its key index (g_attach, g_pxy_pool,
// g_pxy_pool_key: never destructed, emptied with the draw list at both
// mission edges by kh_attach_drop_all); the crew table (g_crew, g_crew_notman
// and its counters g_crew_serial / g_crew_frame: never destructed, emptied at
// both mission edges by kh_crew_clear_all);
// g_mesh_publish_pending and g_mesh_cache_later_q (kh_mesh_release_session); the material pool
// (kh_material_pool_reset keeps the pinned default); the cloth pool's records
// (kh_cloth_workers_stop); the read-memo epoch (bumped, never reset); the presence memo's epoch
// (g_dl_pres_epoch, KH_DL_PRES_MEMO: bumped, never reset - the session reset empties its table); the map
// redirect (g_redir / g_redir_live / g_rbuf, KH_MAP_REDIRECT: a mapping in flight must reach its Unmap, which frees
// its record; the cached buffers are the process's; g_wc_cached_src is raised and lowered around each capture); the
// profiler's
// TSC calibration (g_prof_cal_q / g_prof_cal_t / g_prof_tsc_per_us, KH_PROF_SUB: the machine's rate - the next armed
// fold measures it again over the gap) and its sampling state (g_prof_up_ctr / _on / _t, g_prof_ds_ctr / _on / _t:
// call counters, the bools taken again before every use - g_prof_up_cur, the step in flight, is reset); the GPU pass
// list's entries (g_gpu_pass_pub, KH_GPU_PASS: read only below
// g_gpu_pass_pub_n, which the stats reset zeroes); the fast clock (g_fast_clock, KH_FAST_CLOCK: a new clock origin
// re-anchors it); the want table's role bits (g_dl_keep_kind: written afresh with each new entry); the Eden flag's copy
// (g_attach_eden, KH_ATTACH_EDEN: stored again at the next Draw3D or command read); the game thread's
// id (g_game_tid, KH_RESET_THREAD: the process's, stamped by flush_frame, flush_ui_frame and
// ensure_draw_eh); the collider
// pose serial (g_physics_pose_serial: an identity - 32-bit, skipping 0, so it repeats only 2^32 poses on, and
// every reader compares a stamp it took from it with ==); the
// vertex-stage hint (g_uvs_sets, KH_USER_VS: the deferred teardown runs under
// the NEXT mission's objects, whose sets the parse has already counted - zeroed
// here it would switch their vertex stages off; left standing it costs a
// later mission without one a walk of the live scene a flush); the
// interned path strings (a name -> pointer table nothing resolves through) and
// effect texture sets (g_fx_texset_pool, KH_FX_TEX: immutable records objects
// point at);
// the compiled proxy handlers; the per-thread draw handshakes (set and
// consumed inside one draw); the bind record (t_kh_bc, KH_BIND_FILTER: per
// thread, believed only inside a pass's scope, forgotten at both its ends);
// the upload scratch (written before every read); the hook install's
// constant-buffer reuse verdict (g_rp_cbreuse_on: set where the hooks are
// installed, as the install records); KH_VM_SEE's re-issue saves
// (g_vmsee_sv_*: null outside one draw's kh_vmsee_t_begin / _end pair);
// g_ui_frame_rtv / g_ui_ps_learn (kh_ui_mask_reset, below, releases and clears
// them). Runs first in reset_session_state, after every device object is
// released, so no COM pointer is dropped unreleased; the hand-written resets
// below it still decide the values they set.
// KH_VOL_REPLAY, KH_MIR_REPLAY, KH_SEAM_WITHHOLD, KH_GTS_IDENTITY, KH_RT_PAINT_READ: every namespace-scope global of
// these features
// back to a fresh process's value (SESSION CLEAN SLATE). Idempotent: the GPU objects are released first (normally
// already done by release_shadow_device_state), so it is right in either teardown order.
inline void kh_rp_globals_reset() {
    kh_rp_release_all();
    g_rp_merge_cyc = ~0ull;
    g_rp_replay_cyc = ~0ull;   // KH_REPLAY_ONCE.
    g_rp_mir_cyc = ~0ull;   // KH_MIR_REPLAY (its targets and g_rp_mir_ok go with kh_rp_release_all).
    g_rp_mir_want = false;
    g_rp_mirb_drawn = false;
    g_rp_mir_pend = false;
    g_rp_mir_pend_cyc = g_rp_mir_pend_pass = ~0ull;
    g_rp_sc_all = KhRpRect();   // KH_REPLAY_SCISSOR.
    g_rp_sc_mir = KhRpRect();
    g_rp_sc_gen = ~0ull;
    g_rp_pass_vol = g_rp_pass_mir = false;
    g_rp_rast_on = false;   // KH_REPLAY_RASTER.
    kh_reinit(g_rp_rast);
    g_rp_seam_b = false;
    g_rp_withheld_seq = ~0ull;
    g_rp_front_idx = 0;
    g_rp_front_vp = D3D11_VIEWPORT{};
    g_rp_c = KhRpCounters();
    g_gid_k = 0;
    g_gid_ok = false;
    g_gid_breaks = 0;
    g_rtp_c = KhRtpCounters();
    g_rtp_cyc = ~0ull;
    g_rtp_read_ser = 0;
}
inline void kh_session_globals_reset() {
    kh_rp_globals_reset();   // KH_VOL_REPLAY and its companions.
    g_stats_armed.store(false, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_ticks) khsg_a.store(0, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_count) khsg_a.store(0, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_us_pub) khsg_a.store(0, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_n_pub) khsg_a.store(0, std::memory_order_relaxed);
    for (auto& khsg_a : g_prof_sample_ctr) khsg_a.store(0, std::memory_order_relaxed);   // KH_PROF_SAMPLE.
    for (auto& khsg_a : g_prof_tsc) khsg_a.store(0, std::memory_order_relaxed);   // KH_PROF_SUB.
    for (auto& khsg_a : g_prof_xb_acc) khsg_a.store(0, std::memory_order_relaxed);   // KH_PROF_UPLOAD.
    for (auto& khsg_a : g_fun_acc) khsg_a.store(0, std::memory_order_relaxed);   // KH_PROF_FUNNEL.
    g_prof_up_cur = KHP_ZONE_N;
    g_prof_prev_on = false;   // KH_PROF_WINDOW.
    g_prof_epoch.store(0, std::memory_order_relaxed);   // KH_PROF_DISARM.
    g_prof_epoch_seen = 0;
    kh_reinit(g_gpu_prof);
    for (auto& khsg_a : g_gpu_us_pub) khsg_a.store(0, std::memory_order_relaxed);
    g_cloth_q_n.store(0, std::memory_order_relaxed);
    g_skin_q_n.store(0, std::memory_order_relaxed);   // KH_POOL_SPLIT.
    kh_reinit(g_mat_grave);
    g_eds_memo_res = nullptr;
    g_eds_memo_w = 0;
    g_eds_memo_h = 0;
    kh_reinit(g_res);
    kh_reinit(g_ui_mask);
    g_ui_poison_run = 0;
    g_ui_mask_wanted.store(false, std::memory_order_relaxed);
    g_present_ui_ms.store(0, std::memory_order_relaxed);
    g_present_seq.store(0, std::memory_order_relaxed);
    kh_reinit(g_present_rtv);
    kh_reinit(g_present_rtv_id);
    g_present_rtv_next = 0;
    g_svs_mesh_wanted.store(false, std::memory_order_relaxed);
    g_fogsc_wanted.store(false, std::memory_order_relaxed);
    g_dlf_wanted.store(false, std::memory_order_relaxed);   // KH_DLF_DEMAND.
    g_kh_flush_active.store(false, std::memory_order_relaxed);
    g_ui_mask_injecting.store(false, std::memory_order_relaxed);
    g_kh_track_wanted.store(false, std::memory_order_relaxed);
    g_ui_frame_rtv_seq = 0;
    g_ui_ps_learn_n = 0;
    kh_reinit(g_rain_vel);
    g_rain_phase = 0.0;
    kh_reinit(g_rain_prev_cam);
    g_rain_prev_valid = false;
    g_rain_prev_us = 0;
    g_topo_cycles = 0;
    g_topo_cycles_pub.store(0, std::memory_order_relaxed);
    g_attach_repaired.store(0, std::memory_order_relaxed);
    g_attach_refused.store(0, std::memory_order_relaxed);
    g_attach_skew_bits.store(0, std::memory_order_relaxed);
    g_attach_final_step = false;
    g_sun_restep_cycle = ~0ull;
    g_flush_frame = 0;
    g_flush_frame_pub.store(0, std::memory_order_relaxed);
    kh_reinit(g_stats);
    g_flush_serial.store(0, std::memory_order_relaxed);
    g_scene_depth_samples.store(0, std::memory_order_relaxed);
    g_main_depth_identity.store(nullptr, std::memory_order_relaxed);
    g_main_depth_w = 0;
    g_main_depth_h = 0;
    g_wrong_pass_streak = 0;
    g_flush_wrong_pass = false;
    g_rtshadow_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_park_cycle = ~0ull;
    g_flush_miss_seen_serial = 0;
    g_flush_landed_this_frame = false;
    g_scene_vp_min_d = 0.011f;
    g_scene_vp_max_d = 0.999f;
    g_cb_mirror_obj_b = 0;
    g_cb_mirror_frame_b = 0;
    g_cb_mirror_bad = false;
    g_cbr_supported = false;
    g_kh_ctx1_null_first = false;
    g_cb_offsetting = false;
    g_scene_hold = false;   // KH_SCENE_HOLD (scoped; false outside a flush).
    kh_reinit(g_vis_memo_inj);
    kh_reinit(g_vis_memo_flush);
    kh_reinit(g_dist_memo_inj);
    kh_reinit(g_dist_memo_flush);
    kh_reinit(g_ord_keys);
    kh_reinit(g_ord_tmp);
    kh_reinit(g_cast_key_scr);   // KH_SKIN_POOL: the pooled sorts' tie-break scratch.
    g_shader_cache_hits.store(0, std::memory_order_relaxed);
    g_shader_cache_misses.store(0, std::memory_order_relaxed);
    kh_reinit(g_physics_bvh);
    g_physics_bvh_frame = 0;
    kh_reinit(g_cloth);
    kh_reinit(g_cloth_q);
    kh_reinit(g_cloth_grave);
    g_cloth_skipped.store(0, std::memory_order_relaxed);
    g_simlod_seq.store(0, std::memory_order_relaxed);   // KH_SIM_LOD: no view until the next boundary.
    for (auto& khsg_c : g_simlod_cam) khsg_c.store(0.0f, std::memory_order_relaxed);
    g_simlod_m11.store(0.0f, std::memory_order_relaxed);
    g_simlod_t.store(-1.0e30f, std::memory_order_relaxed);
    g_simlod_work.store(0, std::memory_order_relaxed);
    g_simlod_full.store(0, std::memory_order_relaxed);
    kh_reinit(g_cloth_warned);
    g_cloth_shape_serial.store(0, std::memory_order_relaxed);
    kh_reinit(g_skin_q);
    kh_reinit(g_skin_col);
    g_skin_col_stamp = 0;
    g_skin_col_pub = 0;
    g_skin_col_lo = 0;
    g_skin_col_hi = 0;
    kh_reinit(g_skin_col_snap);
    kh_reinit(g_cloth_vb_slot);
    kh_reinit(g_cloth_mesh_slot);
    kh_reinit(g_cloth_gen_slot);
    kh_reinit(g_cloth_sim_slot);   // KH_LOD_SIM_SLOT.
    kh_reinit(g_cloth_voff_slot);   // KH_SKIN_POOL.
    kh_reinit(g_cloth_pool_slot);
    kh_reinit(g_cloth_objs_h);
    g_chain_skipped.store(0, std::memory_order_relaxed);
    kh_reinit(g_physics_col);
    g_physics_col_n.store(0, std::memory_order_relaxed);
    kh_reinit(g_cloth_objs);
    { const float khsg_v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_cloth_wind, khsg_v, sizeof(g_cloth_wind)); }
    g_cloth_dt_last = 0.0;
    kh_reinit(g_skin);
    kh_reinit(g_skin_rest);
    g_skin_gen_serial.store(0, std::memory_order_relaxed);
    g_skin_skipped.store(0, std::memory_order_relaxed);
    g_skin_pool_full.store(0, std::memory_order_relaxed);   // skinPoolFull.
    kh_reinit(g_skin_gpu);
    kh_reinit(g_skin_pool);   // KH_SKIN_POOL.
    g_skin_pool_serial = 0u;
    g_skin_pool_failed = false;
    g_skin_pool_made = false;   // KH_SKIN_POOL_MADE.
    g_skin_pool_any = false;
    g_skin_so_failed.store(false, std::memory_order_relaxed);   // KH_SKIN_GPU.
    g_skin_so_retry_at = ~0ull;   // KH_SKIN_SO_RETRY.
    g_skin_so_n.store(0, std::memory_order_relaxed);
    g_skin_cpu_n.store(0, std::memory_order_relaxed);
    g_skin_cull_n.store(0, std::memory_order_relaxed);   // KH_SKIN_CULL.
    g_skin_stat_cycle = ~0ull;
    g_skin_stat_gpu = 0u;
    g_skin_stat_cpu = 0u;
    g_skin_stat_cull = 0u;
    g_skin_cull_any = false;   // KH_SKIN_CULL: the attachments' flags went with them.
    g_skin_cull_rad_lo = 0.0f;
    g_skin_cull_rad_hi = 0.0f;
    g_skin_cull_pip_cycle = ~0ull;
    g_skin_bound_worst_bits.store(0, std::memory_order_relaxed);
    kh_reinit(g_skin_rest_gpu);   // Its buffers went with kh_skin_release_all.
    kh_reinit(g_tex_pages);
    kh_reinit(g_tex_cache);
    kh_reinit(g_user_ps_cache);
    kh_reinit(g_user_req);
    kh_reinit(g_user_vtx_scan);   // KH_USER_VS.
    kh_reinit(g_uvs);             // Its buffers went with kh_uvs_release_all.
    g_uvs_key_serial = 0u;
    g_user_req_seen = 0;
    g_user_req_settle = 0;
    g_user_req_waits = 0;
    g_user_batch_own = false;
    kh_reinit(g_user_lut_cache);
    g_lod_proj_px = 0.0f;
    kh_reinit(g_khmw_q);
    g_khmw_running = false;
    g_khmw_abort.store(false, std::memory_order_relaxed);
    g_khmw_busy_id.store(-1, std::memory_order_relaxed);
    kh_reinit(g_khtl_q);
    kh_reinit(g_khtl_done);
    g_khtl_running = false;
    g_khtl_abort.store(false, std::memory_order_relaxed);
    g_vmir_vs = nullptr;
    g_khsg_scale.store(0.5f, std::memory_order_relaxed);
    g_khsg_w = 0;
    g_khsg_h = 0;
    g_comp_fail_streak = 0;
    g_comp_next_retry = 0.0f;
    kh_reinit(g_comp_last_err);
    g_sun_size_div.store(1, std::memory_order_relaxed);
    g_sun_map_size = KH_SUN_DEPTH_BASE;
    g_sun2_map_size = KH_SUN_HERO_BASE;
    g_sun3_map_size = KH_SUN_MID_BASE;
    g_sun4_map_size = KH_SUN_OUT_BASE;
    g_sun5_map_size = KH_SUN_FAR_BASE;
    g_sun_range.store(200.0f, std::memory_order_relaxed);
    g_obj_vis.store(0.0f, std::memory_order_relaxed);
    g_sun_range_src.store(0, std::memory_order_relaxed);
    g_shadows_off.store(false, std::memory_order_relaxed);   // KH_SHADOW_OFF.
    g_sun_pf_autogen = false;
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_composite_last_inject_ms.store(0, std::memory_order_relaxed);
    g_composite_inject_serial.store(0, std::memory_order_relaxed);
    g_composite_land_cycle = ~0ull;
    kh_reinit(g_last_pv);
    g_last_pv_ms = 0;
    g_proj_locator_ever = false;
    kh_reinit(g_khfx_pair);
    g_khfx_pair_near = -1.0f;
    g_khfx_cand_near = -1.0f;
    g_khfx_pair_ms = 0;
    g_khfx_verdict_serial = ~0ull;
    g_khfx_verdict_held = false;
    g_vm_slice_live.store(false, std::memory_order_relaxed);
    g_vm_slice_known.store(false, std::memory_order_relaxed);   // KH_SHOW_INFRONT.
    g_vm_watch_cycle = false;
    g_snap_serial = 0;
    g_snap_ms = 0;
    kh_reinit(g_inj_dp);
    g_inj_dp_valid = false;
    { const float khsg_v[2] = { 0.0f, 0.0f }; memcpy(g_snap_pair, khsg_v, sizeof(g_snap_pair)); }
    g_snap_pair_src = 0;
    { const float khsg_v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_snap_cam, khsg_v, sizeof(g_snap_cam)); }
    kh_reinit(g_snap_vp);
    g_snap_vp_lo = 0.011f;
    g_snap_vp_hi = 0.999f;
    g_snap_vp_valid = false;
    kh_reinit(g_thm_data);
    g_thm_w = 0;
    g_thm_h = 0;
    kh_reinit(g_thm_origin);
    g_thm_cell = 0.0f;
    g_thm_split = KH_THM_SPLIT_SMOOTH;   // KH_THM_EXACT.
    g_thm_dirty.store(false, std::memory_order_relaxed);
    g_thm_valid.store(false, std::memory_order_relaxed);
    g_thm_reupload.store(false, std::memory_order_relaxed);   // KH_THM_DEVRESET.
    kh_reinit(g_thml_origin);
    g_thml_cell = 0.0f;
    g_thml_split = KH_THM_SPLIT_SMOOTH;   // KH_THM_EXACT.
    g_thml_w = 0;
    g_thml_h = 0;
    g_thm_auto_state = 0;
    g_thm_auto_cursor = 0;
    g_thm_auto_retry = 0;
    g_thmf_dim = 0;
    g_thmf_cell = 0.0f;
    kh_reinit(g_voc_thm_max);
    kh_reinit(g_voc_thm_fine);
    g_voc_thm_w = 0;
    g_voc_thm_fw = 0;
    g_voc_thm_fh = 0;
    kh_reinit(g_voc_thm_origin);
    g_voc_thm_cell = 0.0f;
    g_voc_thm_valid = false;
    g_reorder_render_tid.store(0, std::memory_order_relaxed);
    g_watch_clears.store(0, std::memory_order_relaxed);   // KH_WATCH_STALL (and the three below).
    g_watch_cyc_seen = ~0ull;
    g_watch_clears_seen = 0;
    g_watch_still = 0;
    kh_reinit(g_ro);
    g_hook_exceptions.store(0, std::memory_order_relaxed);
    g_slot_keep_m22 = 0.0f;
    g_slot_keep_m32 = 0.0f;
    g_slot_keep_near = -1.0f;
    g_slot_keep_ms = 0;
    kh_reinit(g_cbc);
    kh_reinit(g_latch_cam);
    g_latch_cam_valid = false;
    g_cam_step_m = -1.0f;
    g_boundary_pv_valid = false;
    g_boundary_t = -1.0f;
    { const float khsg_v[3] = { -1.0f, -1.0f, -1.0f }; memcpy(g_fog_uw_col, khsg_v, sizeof(g_fog_uw_col)); }
    kh_reinit(g_proj_locator);
    kh_reinit(g_proj_pending);
    g_proj_last_m32 = 0.0f;
    kh_reinit(g_upc);   // KH_UPLOAD_LAZY.
    kh_reinit(g_uwc);
    g_sun_valid = false;
    { const float khsg_v[3] = { 0.0f, 1.0f, 0.0f }; memcpy(g_sun_dir_engine, khsg_v, sizeof(g_sun_dir_engine)); }
    g_fog_staged_valid = false;
    { const float khsg_v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_fog_staged, khsg_v, sizeof(g_fog_staged)); }
    g_fog_valid = false;
    { const float khsg_v[3] = { 0.0f, 0.0f, 0.0f }; memcpy(g_fog, khsg_v, sizeof(g_fog)); }
    g_sun_jump_pending = false;
    { const float khsg_v[3] = { 0, 0, 0 }; memcpy(g_skysun_ref, khsg_v, sizeof(g_skysun_ref)); }
    g_skysun_ref_valid = false;
    g_skysun_ref_ms = 0;
    kh_reinit(g_skysun_mot_ref);
    g_skysun_mot_ref_ms = 0;
    g_skysun_mot_deg = -1.0f;
    g_atlas_frame_cur = false;
    g_atlas_frame_prev = false;
    g_pub_valid = false;
    kh_reinit(g_pub_dir);
    g_pub_block_valid = false;
    { const float khsg_v[8] = { -1,-1,-1,-1,-1,-1,-1,-1 }; memcpy(g_band_tab_far, khsg_v, sizeof(g_band_tab_far)); }
    kh_reinit(g_pub_block_amb);
    kh_reinit(g_pub_block_sun);
    g_lite_cand_n = 0;
    g_lite_cand_sl = 0.0f;
    g_lite_cand_al = 0.0f;
    g_blko_draws = 0;
    kh_reinit(g_blko_cur);
    g_blko_n = 0;
    g_blko_open_draws = 0;
    g_blko_open = false;
    g_blko_open_i = -1;
    kh_reinit(g_blko_open_amb);
    kh_reinit(g_blko_open_sun);
    kh_reinit(g_blko_last);
    kh_reinit(g_blko_gen_amb);   // KH_DLSW_GEN_AMB.
    kh_reinit(g_blko_gen_sun);
    g_blko_gen_valid = false;
    kh_reinit(g_sraw_cyc);
    g_sraw_cyc_n = 0;
    g_sraw_last_sample_ms = 0;
    g_sraw_last_src = 0;
    kh_reinit(g_sraw_sky_prev);
    g_sraw_sky_prev_valid = false;
    g_sraw_sky_step_deg = -1.0f;
    g_sraw_pub_gap_deg = -1.0f;
    g_sraw_sky_gap_deg = -1.0f;
    kh_reinit(g_sraw_ring);
    g_sraw_ring_n = 0;
    g_sraw_ring_i = 0;
    kh_reinit(g_sraw_med_prev);
    g_sraw_med_prev_valid = false;
    g_sraw_hold_until_ms = 0;
    g_sraw_cons_moved_ms = 0;
    g_sraw_unsettled_ms = 0;
    kh_reinit(g_ls);
    g_ls_cam_cycle = ~0ull;
    kh_reinit(g_inj_view);
    g_inj_view_ms = 0;
    kh_reinit(g_light_probe);
    kh_reinit(g_sky_probe);
    kh_reinit(g_skybind);
    g_ring_best_valid = false;
    g_acq_hold.store(false, std::memory_order_relaxed);
    g_ring_best_er = 0.0f;
    g_ring_best_res = nullptr;
    g_ring_best_off = 0;
    g_ring_best_orient = 1;
    g_ring_best_cycle = 0;
    kh_reinit(g_ring_best_m);
    g_pub_first = -1.0f;
    g_prewarm_cand_res = nullptr;
    g_prewarm_cand_off = 0;
    g_coldlock_cand_res = nullptr;
    g_coldlock_cand_off = 0;
    g_cc_flush_serial.store(0, std::memory_order_relaxed);
    g_cc_flush_opaques.store(0xFFFFFFFFu, std::memory_order_relaxed);
    g_inj_enc_m22 = 0.0f;
    g_inj_enc_m32 = 0.0f;
    g_inj_enc_near = -1.0f;
    g_inj_enc_ms = 0;
    g_ms_flush_serial = 0;
    g_carry_pending_serial.store(0, std::memory_order_relaxed);
    g_rescue_frame_count = 0;
    g_kh_ctx1 = nullptr;
    g_kh_ctx1_for = nullptr;
    kh_reinit(g_rt0_desc);
    g_rt0_desc_n = 0;
    g_rt0_desc_next = 0;
    g_shadow_map_strength = 1.0f;
    g_shadow_map_bias = 0.0015f;
    g_shadow_map_sign = 1.0f;
    g_sun_map_rendered_frame = false;
    g_sun_map_valid = false;
    g_sun_map_cam_anchor = false;
    g_sun_map_render_time = -1.0f;
    g_sun_mat_chash = 0;
    kh_reinit(g_sun_mat_sun);
    g_sun_map_time = -1.0f;
    g_sun_map_no_local = false;
    kh_reinit(g_sun_map_vp);
    kh_reinit(g_sun_anchor_now);
    kh_reinit(g_sun_cam_now);   // KH_DLS_LOD_CAM.
    kh_reinit(g_sun_map_anchor);
    kh_reinit(g_sun_cam_anchor);
    kh_reinit(g_sun_tier_anchor);
    kh_reinit(g_sun_tier_lat);   // KH_PCSS_CELL.
    g_vmir_w = 0;
    g_vmir_h = 0;
    g_vmir_mask_time = -1.0f;
    g_sun_map_bias = 0.0f;
    g_sun_map_cast_bias = 0.0f;
    kh_reinit(g_sun2_map_vp);
    g_sun2_map_bias = 0.0f;
    g_sun2_cast_bias = 0.0f;
    g_sun2_half_diag = 0.0f;
    g_sun2_map_valid = false;
    g_sun2_renders = 0;
    g_sun2_casters = 0;
    kh_reinit(g_sun3_map_vp);
    g_sun3_map_bias = 0.0f;
    g_sun3_cast_bias = 0.0f;
    g_sun3_half_diag = 0.0f;
    g_sun3_map_valid = false;
    g_sun3_renders = 0;
    g_sun3_casters = 0;
    kh_reinit(g_sun4_map_vp);
    g_sun4_map_bias = 0.0f;
    g_sun4_cast_bias = 0.0f;
    g_sun4_half_diag = 0.0f;
    g_sun4_map_valid = false;
    g_sun4_renders = 0;
    g_sun4_casters = 0;
    kh_reinit(g_sun5_map_vp);
    g_sun5_map_bias = 0.0f;
    g_sun5_cast_bias = 0.0f;
    g_sun5_half_diag = 0.0f;
    g_sun5_map_valid = false;
    g_sun5_renders = 0;
    g_sun5_casters = 0;
    { const bool khsg_v[4] = { false, false, false, false }; memcpy(g_sun_pf_valid, khsg_v, sizeof(g_sun_pf_valid)); }
    { const bool khsg_v[4] = { false, false, false, false }; memcpy(g_sun_pf_fresh, khsg_v, sizeof(g_sun_pf_fresh)); }
    { const bool khsg_v[4] = { false, false, false, false }; memcpy(g_sun_pf_held, khsg_v, sizeof(g_sun_pf_held)); }
    { const uint64_t khsg_v[4] = { 0, 0, 0, 0 }; memcpy(g_sun_tier_key, khsg_v, sizeof(g_sun_tier_key)); }
    { const bool khsg_v[4] = { false, false, false, false }; memcpy(g_sun_tier_key_ok, khsg_v, sizeof(g_sun_tier_key_ok)); }
    g_sun_pf_done = ~0ull;
    { const float khsg_v[4] = { -1.0f, -1.0f, -1.0f, -1.0f }; memcpy(g_sun_band_reach, khsg_v, sizeof(g_sun_band_reach)); }
    { const float khsg_v[4] = { -1.0f, -1.0f, -1.0f, -1.0f }; memcpy(g_sun_fwd_held, khsg_v, sizeof(g_sun_fwd_held)); }
    g_sun_union_rad_held = -1.0f;
    g_sun_union_lat_held = -1.0f;
    g_sun_map_hash = 0;
    kh_reinit(g_sun_map_bounds);
    g_fire_lock = nullptr;
    kh_reinit(g_fire_fov2);
    kh_reinit(g_fire_dims2);
    kh_reinit(g_fire_view2);
    kh_reinit(g_fire_sun2);
    g_fire_lock_valid = false;
    g_fire_post_scene_cycle = ~0ull;
    g_fire_count_frame = 0;
    g_lock_churn_ms_v = 0;
    g_fire_era_reject_run = 0;
    g_cast_ready = false;
    g_cast_ready_since_ms = 0;
    g_cascbind_feed_t = -1.0f;
    g_fire_clamp_target = 1;
    g_fire_clamp_painted = 0;
    kh_reinit(g_cycle_pv_prev);
    g_cycle_pv_prev_valid = false;
    g_cycle_pv_t = -1.0f;
    g_cycle_pv_age_s = -1.0f;
    g_fire_mask_srv_now = false;
    g_view_drop_path = 0;
    g_view_drop_bar = -1.0f;
    g_view_drop_hyst_since = 0;
    g_view_relock_wide_cycle = 0;
    kh_reinit(g_band_pool);
    g_band_fall_flush = ~0ull;
    g_recv_health_ok = false;
    g_recv_health_since_ms = 0;
    kh_reinit(g_fire_refreeze_view);
    g_fire_refreeze_pending = false;
    kh_reinit(g_fire_cast_inv);
    g_fire_cast_inv_valid = false;
    kh_reinit(g_sun_locals);
    g_sun_local_count = 0;
    g_sun_locals_serial = 0;
    g_mask_cast_arm = false;
    g_cast_map_frame = ~0ull;   // KH_CAST_FRAME64.
    g_mission_destroy_pending.store(false, std::memory_order_relaxed);
    g_topo_scene_tex_id = nullptr;
    g_topo_scene_seen_cycle = 0;   // KH_SCENE_NAME.
    g_mask_cast_fired = false;
    g_dl_pool_next_id = 1;
    g_dl_intensity_bits.store(0x3F800000u, std::memory_order_relaxed);
    kh_reinit(g_cbs);
    g_cbs_gen = 0;
    g_cbs_wseq = 0;   // KH_DL_REC_ORDER: with the pool (kh_reinit(g_dl), next), whose records it orders.
    kh_reinit(g_dl);
    g_dl_harvest_cycle = ~0ull;
    g_dl_harvest_flush_last = false;
    kh_reinit(g_dl_cd_bufs);
    kh_reinit(g_dl_cd_first);
    g_dl_cd_n = 0;
    kh_reinit(g_dl_phix);   // KH_DL_PRES_HIX: the pool's height index (with g_dl, above).
    kh_reinit(g_dls);
    g_dls_n = 0;
    g_dls_frame = 0;
    g_dls_sel_ms = 0;   // KH_DLS_POOL_AGE.
    g_dls_allowed.store(false, std::memory_order_relaxed);   // KH_DLS_ALLOW: off.
    kh_reinit(g_dls_keys);
    g_dlr_buf = nullptr;   // KH_DL_RING: released by release_shadow_device_state first.
    g_dlr_srv = nullptr;
    g_dlr_cap = 0;
    g_dlr_cursor = 0;
    g_dlr_fresh = true;
    g_dlr_nooverwrite = -1;
    g_ao_strength_bits.store(0x3F800000u, std::memory_order_relaxed);
    g_ao_dist_bits.store(0x3F000000u, std::memory_order_relaxed);
    kh_reinit(g_sr);
    kh_reinit(g_atlas_srv_cache);
    g_atlas_srv_count = 0;
    kh_reinit(g_cascbind);
    g_cascharv_ctr = 0;
    kh_reinit(g_mask);
    kh_reinit(g_dls_vp);
    kh_reinit(g_dls_facen);
    kh_reinit(g_dls_faceslice);
    kh_reinit(g_dls_facemask);
    kh_reinit(g_dls_isspot);
    kh_reinit(g_dls_face_hold);
    kh_reinit(g_dls_mapfar);
    g_dls_map_valid = 0;
    kh_reinit(g_dlsw_casters);
    g_dls_frame_cycle = ~0ull;
    g_castocc_key = 0;
    kh_reinit(g_castocc_lo);
    g_castocc_inv = 0.0f;
    kh_reinit(g_castocc_grid);
    kh_reinit(g_castocc_set);
    kh_reinit(g_castocc_addi);
    g_castocc_cell = 0.0f;
    kh_reinit(g_castocc_sunb);
    kh_reinit(g_castocc_dom);
    g_castocc_slack = 0.0f;
    g_castocc_rdom = 0.0f;
    g_castocc_rng = 0.0f;   // KH_CASTOCC_RANGE.
    g_castocc_floor = 0.0f;
    kh_reinit(g_castocc_pre);
    g_castocc_pre_noreach = false;
    kh_reinit(g_castocc_pre_noreach_dom);
    g_castocc_kc = 0;
    g_castocc_kc_ser = ~0ull;
    g_castocc_kc_rng = -1.0f;
    g_castocc_kc_n = -1;
    kh_reinit(g_svs_rtv_seen);
    g_svs_rtv_seen_n = 0;
    kh_reinit(g_svs_cls_id);
    kh_reinit(g_svs_cls_srv);
    g_svs_cls_n = 0;
    g_svs_pending = false;
    g_svs_pend_w = 0;
    g_svs_pend_h = 0;
    g_svs_mask_id = nullptr;
    g_svs_mask_srv = nullptr;
    g_svs_mask_w = 0;
    g_svs_mask_h = 0;
    g_svs_mask_fmt = 0;
    g_svs_mask_cand = nullptr;
    g_svs_pre_tex = nullptr;
    g_svs_pre_srv = nullptr;
    g_svs_pre_w = 0;
    g_svs_pre_h = 0;
    g_svs_pre_fmt = 0;
    g_svs_post_tex = nullptr;
    g_svs_post_srv = nullptr;
    g_svs_post_made = 0;
    g_depth_clear_serial = 0;
    g_svs_skip_sig = 0;
    g_svs_skip_valid = false;
    g_svs_frame_seq = 0;
    g_svs_bracket = 0;
    kh_reinit(g_svs_caster_list);
    kh_reinit(g_svs_inst_ord);
    kh_reinit(g_svs_prime_vp);
    g_khpe_p1 = -1.0f;
    g_khpe_p2 = -1.0f;
    g_khpe_m22 = 0.0f;
    g_khpe_ms = 0;
    g_khpe_upd_ms = 0;
    g_khpe_skip1 = -1.0f;
    g_khbl_stable = -1.0f;
    g_khbl_prev = -1.0f;
    kh_reinit(g_khlt_prev);
    g_khlt_prev_ok = false;
    g_khlt_prev_step = 0.0f;
    kh_reinit(g_khlt_step_ring);
    g_khlt_step_ring_i = 0;
    kh_reinit(g_khlt_ref_cam);
    g_khlt_ref_ok = false;
    kh_reinit(g_khlt_lp);
    g_khlt_lp_ok = false;
    kh_reinit(g_svs_prime_cam);
    g_svs_prime_rebase = false;
    g_svs_prime_vp_lo = 0.0f;
    g_svs_prime_vp_hi = 1.0f;
    g_svs_vol_src = nullptr;
    g_svs_vol_src_id = nullptr;
    g_svs_vol_tex = nullptr;
    g_svs_vol_sten_srv = nullptr;
    g_svs_vol_depth_srv = nullptr;   // KH_VOL_WITNESS: released by kh_svs_vol_release first.
    g_svs_vol_w = 0;
    g_svs_vol_h = 0;
    g_svs_vol_primed = false;
    g_svs_vol_seq = 0;
    kh_reinit(g_svs_seam_enc);   // KH_VOL_WITNESS.
    g_svs_seam_enc_seq = ~0ull;
    kh_reinit(g_svs_vol_enc);
    g_svs_vol_enc_ok = false;
    kh_reinit(g_svs_seam_proj);   // KH_VOL_ZOOM.
    g_svs_seam_proj_ok = false;
    kh_reinit(g_svs_vol_proj);
    g_svs_vol_proj_ok = false;
    // KH_VOL_FOOT: released by kh_svs_vol_release (kh_svs_foot_release) first.
    kh_reinit(g_svs_foot_tex);
    kh_reinit(g_svs_foot_rtv);
    kh_reinit(g_svs_foot_srv);
    g_svs_foot_w = 0;
    g_svs_foot_h = 0;
    g_svs_foot_pub = -1;
    g_svs_foot_seam_ix = -1;
    g_svs_foot_seam_seq = ~0ull;
    g_svs_vol_foot_ok = false;
    g_svs_prime_ready = false;
    g_svs_vol_dsv_now = false;
    g_svs_vol_dsv_bound = false;
    g_svs_injected_frame = false;
    kh_reinit(g_engcam_cand);
    g_engcam_cand_n = 0;
    g_engcam_cand_seq = ~0ull;
    g_engcam_res = nullptr;
    g_engcam_off = -1;
    kh_reinit(g_engcam_vals);
    g_engcam_vals_n = 0;
    g_engcam_val_seq = ~0ull;
    kh_reinit(g_engcam_frame_val);
    g_engcam_frame_seq = ~0ull;
    g_engcam_frame_taken = false;
    kh_reinit(g_engcam_ref);
    g_engcam_ref_ok = false;
    g_vmir_tex = nullptr;
    g_vmir_dsv = nullptr;
    g_vmir_srv = nullptr;
    g_vmir_b2_src = nullptr;
    g_vmir_b2_src_srv = nullptr;
    g_vmir_b2_out = nullptr;
    g_vmir_b2_out_rtv = nullptr;
    g_vmir_b2_patch = nullptr;
    g_vmir_b2_vs = nullptr;
    g_vmir_b2_ps = nullptr;
    g_vmir_b2_failed = false;
    g_vmir_pending = false;
    g_vmir_frame_ok = false;
    g_vmir_mark2 = 0;
    g_vmir_prepass_stamp = 0;
    kh_reinit(g_vmir_rs_src);
    kh_reinit(g_vmir_rs_clamp);
    g_vmir_sv_rs = nullptr;
    g_vmir_prepass_src = nullptr;
    kh_reinit(g_b2_snap);
    g_b2_learn = nullptr;
    kh_reinit(g_vmir_sv_rtvs);
    g_vmir_sv_dsv = nullptr;
    kh_reinit(g_vmir_sv_vps);
    g_vmir_sv_nvp = 0;
    g_vmir_sv_b2 = nullptr;
    g_vmir_sv_b2_first = 0;
    g_vmir_sv_b2_num = 0;
    g_vmir_sv_b2_off_ok = false;
    g_pip_seen = false;
    kh_reinit(g_pip);
    g_infront_wanted.store(false, std::memory_order_relaxed);
    g_vm_watch_wanted.store(false, std::memory_order_relaxed);   // KH_SHOW_INFRONT.
    g_vm_prev_near = false;
    g_vm_pre_done = false;
    g_vm_col_done = false;
    g_vm_seam_done = false;
    g_vm_frame_valid = false;
    g_vm_frame_cycle = 0;
    kh_reinit(g_vm_view_proj);
    kh_reinit(g_vm_proj);   // KH_MIR_ZOOM.
    kh_reinit(g_vm_cam);
    g_vm_rebase = 0.0f;
    g_vm_m22 = 0.0f;
    g_vm_m32 = 0.0f;
    g_vm_keep_m22 = 0.0f;
    g_vm_keep_m32 = 0.0f;
    g_vm_keep_ms = 0;
    kh_reinit(g_vm_meshes);
    g_inj_arb_prev_armed = false;
    g_dls_world_cycle = 0;
    g_fx_late_owed_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_ran_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_dry_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_streak.store(0, std::memory_order_relaxed);
    g_fx_late_backoff.store(0, std::memory_order_relaxed);
    g_fx_late_miss_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_miss_cycle2.store(~0ull, std::memory_order_relaxed);
    g_depth_readopt_run = 0;
    kh_reinit(g_hdc_blend);
    kh_reinit(g_hdc_dss);
    g_flush_fx_arb_prev = false;
    g_video_opt_ms = 0;
    g_dl_cell.store(0.0f, std::memory_order_relaxed);   // KH_DL_ORIGIN_GRID: the next session reads its terrain.
    g_dl_q_cells = 0;
    g_dl_stall_a = 0;   // KH_DL_STALL_GAP.
    g_dl_stall_b = 0;
    g_ui_rehoist_auto_s = -1.0e9f;
    g_ui_prev_scene_frame = 0;
    g_ui_prev_from_present = false;   // KH_UI_ORDINAL_SRC.
    g_ui_rehoist_last_s = -1.0e9f;
}

// KH_SESSION_RESET - the globals the objects of a mission hold on to (scene
// slots, handles and sequence numbers, attachment and helper-pool books,
// affectors, cloth proxies and chain settings, the event-handler flags, the
// mesh path keys and reference times, the material table, the effect clock),
// to their declared start the same way. Only when no object exists
// (kh_session_teardown's clean path): the deferred destroy runs under the next
// mission's objects, which these belong to.
inline void kh_session_objects_reset() {
    kh_reinit(g_scene);
    g_scene_unpub_min = 0xFFFFFFFFu;   // KH_LOD_BOUNDS.
    kh_reinit(g_scene_stage);
    kh_reinit(g_scene_handle);
    kh_reinit(g_scene_dirty_mark);
    kh_reinit(g_scene_dirty);
    g_scene_dirty_n.store(0u, std::memory_order_relaxed);   // KH_SYNC_IDLE.
    kh_reinit(g_scene_free);
    kh_reinit(g_objbuf_cpu);
    kh_reinit(g_objbuf_dirty);
    kh_reinit(g_objbuf_dirty_mark);
    g_objbuf_shoff = false;   // KH_SHADOW_OFF.
    g_gts_seq = 0;   // KH_ATTACH_GT_SNAP: the bindings' sample numbers (their rings, picks and helper stamps).
    g_skel_gen_serial = 0;
    g_attach_n.store(0, std::memory_order_relaxed);
    kh_reinit(g_attach_dead);
    g_attach_dead_n.store(0, std::memory_order_relaxed);
    kh_reinit(g_attach_proxy_dead);
    g_attach_proxy_dead_n.store(0, std::memory_order_relaxed);
    g_pxy_pool_n.store(0, std::memory_order_relaxed);
    g_gts_done_pub.store(0, std::memory_order_relaxed);   // Names a g_gts_seq value: restarts with it.
    g_gts_done_defer = 0;   // KH_CHAIN_END_DRAW.
    for (auto& khsg_st : g_gts_stamp_pub) khsg_st.store(0, std::memory_order_relaxed);
    g_next_seq = 0;
    kh_reinit(g_chain_cfg);
    kh_reinit(g_chain_end);   // KH_CHAIN_END_DRAW (reset_retained_state has emptied it).
    g_chain_end_n.store(0, std::memory_order_relaxed);
    kh_reinit(g_chain_warned);
    kh_reinit(g_chain_nobuild);
    g_ground_ready = false;
    g_ground_tried = false;
    g_ground_cell = 0.0f;
    g_ground_n = 0;
    { const uint8_t khsg_v[4] = { 2, 2, 2, 2 }; memcpy(g_ground_split, khsg_v, sizeof(g_ground_split)); }
    kh_reinit(g_affectors);
    g_affectors_n.store(0, std::memory_order_relaxed);
    g_aff_next_seq = 0;
    kh_reinit(g_aff_frame);
    g_tex_page_gen = 0;
    g_tex_land_serial = 0;   // KH_TEX_LAND.
    kh_reinit(g_fbx_cache);
    kh_reinit(g_mesh_last_ref);
    kh_reinit(g_ref_note_cen);   // KH_REF_NOTE_ONCE.
    kh_reinit(g_mat_note_cen);   // KH_MAT_NOTE_ONCE.
    g_ref_note_cen_ms.store(0, std::memory_order_relaxed);   // KH_GC_NOTE_SKIP.
    kh_reinit(g_ref_note_gc);
    kh_reinit(g_mat_gpu_cpu);
    g_mat_gpu_dirty = false;
    g_mat_gpu_page_gen = 0;
    kh_reinit(g_mat_anim);   // KH_UV_ANIM: the list, its two cycle stamps and the cycle's time.
    g_mat_anim_done = ~0ull;
    g_mat_anim_tcyc = ~0ull;
    g_mat_anim_t = 0.0;
    g_effect_t0.store(0, std::memory_order_relaxed);
    kh_reinit(g_cloth_proxy);
    kh_reinit(g_cloth_proxy_want);
    g_cloth_proxy_frame = 0;
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_draw3d_eh_active = false;
}

// Device objects and locator/probe state go through release_shadow_device_state
// + g_res.release (the caller runs those first, under the graphics lock); this
// resets what those don't. Must run with the render thread parked: g_ro, the
// mask/live structs and the hoisted memories are render-thread state.
inline void reset_session_state() {
    kh_session_globals_reset();   // KH_SESSION_RESET: everything to its declared start; the lines below refine it.
    reset_stat_counters();
    kh_session_scratch_reset();
    kh_ui_mask_reset();   // Learned backbuffers, phase machine, demand flag.
    g_kh_track_wanted.store(false, std::memory_order_relaxed);   // Recomputes at the next flush.
    g_infront_wanted.store(false, std::memory_order_relaxed);   // KH_INFRONT: likewise.
    g_vm_watch_wanted.store(false, std::memory_order_relaxed);   // KH_SHOW_INFRONT.
    kh_infront_frame_reset();
    g_vm_keep_ms = 0;
    g_vm_meshes.clear();
    g_ao_strength_bits.store(0x3F800000u, std::memory_order_relaxed);   // KH_SSAO: on, 1.0.
    g_ao_dist_bits.store(0x3F000000u, std::memory_order_relaxed);       // Radius 0.5 m.

    {   // Error dedupe the session (a fresh session re-reports).
        std::lock_guard<std::mutex> g(g_err_once_mutex);
        g_err_once.clear();
        g_err_once_capped = false;   // KH_ERR_ONCE_CAP.
    }

    g_sun_union_rad_held = -1.0f;
    g_sun_union_lat_held = -1.0f;
    for (int khfl_i = 0; khfl_i < 4; ++khfl_i) g_sun_fwd_held[khfl_i] = -1.0f;
      g_ls.wipe_deferred = false;
    // Cleared here, re-armed by the first stage_video_options.
    g_obj_vis.store(0.0f, std::memory_order_relaxed);
    g_video_opt_ms = 0;   // KH_VIDEO_OPT_CADENCE: the next poll is immediate.
    g_dl_cell.store(0.0f, std::memory_order_relaxed);   // KH_DL_ORIGIN_GRID.
    g_dl_q_cells = 0;
     g_cascbind_feed_t = -1.0f;

    // The lifetime pairing trio is session-scoped under full destroy.
      g_ls.view_locks = 0;
    g_lock_churn_ms_v = 0;   // lock-settle: a fresh session starts unheld.
    g_cast_ready = false;   // Readiness latch: a fresh session re-dwells.
    g_cast_ready_since_ms = 0;
    g_recv_health_ok = false; g_recv_health_since_ms = 0;

    g_ls.pub_rej_streak = 0; g_ls.view_src_miss = 0;
    g_ls.last_publish_rot_err = 1.0f;
    g_ls.pub_exact_ms = 0;
    g_ls.pub_fresh_ms = 0;

    g_ls.cam[0] = g_ls.cam[1] = g_ls.cam[2] = 0.0f;
    g_ls.stamp_counter = 0;
    // Published / staged lighting.
    g_sun_valid = false;
    g_sun_dir_engine[0] = 0.0f; g_sun_dir_engine[1] = 1.0f; g_sun_dir_engine[2] = 0.0f;
    g_blko_draws = 0; g_blko_n = 0; g_blko_open = false; g_blko_open_i = -1;   // KH_LIGHT_BASE.
    g_sraw_cyc_n = 0;
    g_sraw_last_sample_ms = 0;
    g_sraw_last_src = 0; g_sraw_pub_gap_deg = -1.0f; g_sraw_sky_gap_deg = -1.0f;
    kh_pip_reset();   // KH_PIP.
    g_sraw_sky_prev_valid = false; g_sraw_sky_step_deg = -1.0f; g_sraw_sky_prev[0] = g_sraw_sky_prev[1] = g_sraw_sky_prev[2] = 0.0f;
    g_sraw_hold_until_ms = 0; g_sraw_cons_moved_ms = 0; g_sraw_unsettled_ms = 0; kh_sraw_ring_reset();   // KH_SRAW_SETTLE.
    g_sun_jump_pending = false;
    g_skysun_ref_valid = false;
    g_skysun_ref[0] = g_skysun_ref[1] = g_skysun_ref[2] = 0.0f;
    g_skysun_ref_ms = 0;
    g_skysun_mot_ref_ms = 0; g_skysun_mot_deg = -1.0f;
    g_pub_valid = false;
    g_pub_dir[0] = g_pub_dir[1] = g_pub_dir[2] = 0.0f;
    g_fog_staged_valid = false; g_fog_valid = false;

    for (int k = 0; k < 3; ++k) { g_fog[k] = 0.0f; g_fog_staged[k] = 0.0f; }

    g_pub_first = -1.0f;
    g_pub_block_valid = false;
    g_pub_block_amb[0] = g_pub_block_amb[1] = g_pub_block_amb[2] = 0.0f;
    g_pub_block_sun[0] = g_pub_block_sun[1] = g_pub_block_sun[2] = 0.0f;
    g_blko_gen_valid = false;   // KH_DLSW_GEN_AMB: goes with the publish.
    g_blko_gen_amb[0] = g_blko_gen_amb[1] = g_blko_gen_amb[2] = 0.0f;
    g_blko_gen_sun[0] = g_blko_gen_sun[1] = g_blko_gen_sun[2] = 0.0f;
    g_lite_cand_n = 0; g_lite_cand_sl = 0.0f; g_lite_cand_al = 0.0f;   // KH_LIT_EVIDENCE.
    g_prewarm_cand_res = nullptr; g_prewarm_cand_off = 0;
    g_coldlock_cand_res = nullptr; g_coldlock_cand_off = 0;
    g_last_pv_ms = 0;
    g_cc_flush_opaques.store(0xFFFFFFFFu, std::memory_order_relaxed);
    g_slot_keep_m22 = 0.0f; g_slot_keep_m32 = 0.0f;
    g_slot_keep_near = -1.0f; g_slot_keep_ms = 0;
    g_latch_cam_valid = false;
    g_latch_cam[0] = g_latch_cam[1] = g_latch_cam[2] = 0.0f;
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_ro = ReorderState{};
    g_rescue_frame_count = 0;
    g_atlas_frame_cur = false; g_atlas_frame_prev = false;

    g_dl_intensity_bits.store(0x3F800000u, std::memory_order_relaxed);
    dynlights_reset_session();

    g_inj_enc_m22 = 0.0f; g_inj_enc_m32 = 0.0f; g_inj_enc_near = -1.0f; g_inj_enc_ms = 0;
    g_khpe_p1 = -1.0f; g_khpe_p2 = -1.0f; g_khpe_m22 = 0.0f;
    g_khpe_ms = 0; g_khpe_upd_ms = 0; g_khpe_skip1 = -1.0f;
    g_khbl_stable = -1.0f; g_khbl_prev = -1.0f;
    g_khlt_prev_ok = false; g_khlt_prev_step = 0.0f;
    g_khlt_lp_ok = false;
    g_khlt_ref_ok = false;

    g_khfx_pair_near = -1.0f; g_khfx_cand_near = -1.0f; g_khfx_pair_ms = 0;
    g_khfx_verdict_serial = ~0ull; g_khfx_verdict_held = false;
    g_boundary_pv_valid = false;   // Session teardown: no stale cross-mission sample.
    g_boundary_t = -1.0f;
    g_carry_pending_serial.store(0, std::memory_order_relaxed);
    g_sr = ShadowReconState{};
    g_reorder_render_tid.store(0, std::memory_order_relaxed);
    g_watch_clears.store(0, std::memory_order_relaxed);   // KH_WATCH_STALL (and the three below).
    g_watch_cyc_seen = ~0ull;
    g_watch_clears_seen = 0;
    g_watch_still = 0;
    g_main_depth_w = 0; g_main_depth_h = 0; g_wrong_pass_streak = 0;
    g_proj_last_m32 = 0.0f;
    g_mask = ShadowMaskState{};   // Device pointers already nulled by the release.
    g_proj_locator_ever = false;
    g_comp_fail_streak = 0; g_comp_next_retry = 0.0f; g_comp_last_err.clear();
    g_snap_serial = 0; g_snap_ms = 0;
    g_fx_park_cycle = ~0ull;   // The cycle counter restarts with the session.
    // KH_FX_LATE: g_topo_cycles restarts with the session, so a stale backoff
    // would otherwise hold the chain on the scene flush for the whole session.
    g_fx_late_owed_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_ran_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_dry_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_streak.store(0, std::memory_order_relaxed);
    g_fx_late_backoff.store(0, std::memory_order_relaxed);
    g_fx_late_miss_cycle.store(~0ull, std::memory_order_relaxed);
    g_fx_late_miss_cycle2.store(~0ull, std::memory_order_relaxed);
    g_flush_miss_seen_serial = 0; g_flush_landed_this_frame = false;   // KH_FLUSH_MISS.
    g_composite_land_cycle = ~0ull;   // The cycle counter restarts with the session.
    g_snap_pair_src = 0;
    g_snap_vp_valid = false;
    g_thm_data.clear();
    g_thm_data.shrink_to_fit();
    g_thm_w = g_thm_h = 0;
    g_thm_origin[0] = g_thm_origin[1] = 0.0f;
    g_thm_cell = 0.0f;
    g_thm_split = KH_THM_SPLIT_SMOOTH;   // KH_THM_EXACT.
    g_thm_dirty = false;
    g_thm_valid = false;
    g_thm_reupload = false;   // KH_THM_DEVRESET.
    g_voc_thm_max.clear();
    g_voc_thm_max.shrink_to_fit();
    g_voc_thm_fine.clear();
    g_voc_thm_fine.shrink_to_fit();
    g_voc_thm_w = 0;
    g_voc_thm_fw = g_voc_thm_fh = 0;
    g_voc_thm_origin[0] = g_voc_thm_origin[1] = 0.0f;
    g_voc_thm_cell = 0.0f;
    g_voc_thm_valid = false;
    g_thml_origin[0] = g_thml_origin[1] = 0.0f;
    g_thml_cell = 0.0f;
    g_thml_split = KH_THM_SPLIT_SMOOTH;   // KH_THM_EXACT.
    g_thml_w = g_thml_h = 0;
    g_thm_auto_state = 0;
    g_thm_auto_cursor = 0;
    g_thm_auto_retry = 0;
    g_thmf_dim = 0;
    g_thmf_cell = 0.0f;
    g_fog_uw_col[0] = g_fog_uw_col[1] = g_fog_uw_col[2] = -1.0f;
}

inline void reset_retained_state() {
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        for (auto& kv : g_draw_list) kh_scene_stage_remove(kv.second);   // KH_SCENE: deaths sync later.
        kh_attach_drop_all(false);   // KH_ATTACH: a mission edge must not hold an object to process exit.
        g_draw_list.clear();
        g_next_seq = 0;   // Creation order restarts with the list it orders.
    }
    kh_crew_clear_all();   // KH_CREW_LAG: outside the mutex (its entries' event handlers go).
    kh_aff_clear();   // KH_AFFECTOR: game thread, like the table itself.
    // KH_CLOTH_PROXY: released, not deleted - the mission's end takes its own
    // objects, and a mission edge must not hold one (kh_attach_drop_all).
    g_cloth_proxy.clear();
    g_cloth_proxy_want.clear();
    g_chain_cfg.clear();   // KH_CHAIN: its proxies too are the mission's, released the same way.
    {   // KH_CHAIN_END_DRAW: the render thread reads the table under the mutex.
        std::lock_guard<std::mutex> khce_g(g_draw_list_mutex);
        g_chain_end.clear();
        g_chain_end_n.store(0, std::memory_order_relaxed);
    }
    g_chain_warned.clear();
    g_chain_nobuild.clear();
    g_aff_frame.clear();
    g_aff_next_seq = 0;
    // KH_CLOTH_GROUND: measured again on the next mission's matrix.
    g_ground_tried = false;
    g_ground_ready = false;
}

// KH_SESSION_RESET - the whole teardown, under the park, for both mission-edge
// paths: the joinable workers first (texture loader, mesh cache writer, the
// cloth / skin pool - nothing may run on after its data is gone), then every
// device object and learned identity, the meshes (into the grave the resource
// release frees), the instances, the caches (textures, user .hlsl, user .cube,
// materials, asset paths), the shader pool (bounded wait, then detach - a job
// inside D3DCompile cannot be killed), and last the session state, which puts
// every other global back to what a fresh process holds. khst_clean: no object
// exists (rendering_integration_reset, after kh_mission_handles_reset). False
// for kh_park_run's deferred destroy, which runs under the next mission's
// objects: what they hold (meshes, materials, scene slots, handles, the effect
// clock) is left to them.
inline void kh_session_teardown(bool khst_clean) {
    kh_texldr_stop();   // KH_TEX_ASYNC: joined, results dropped.
    kh_mesh_cache_writer_stop();   // Joined, queued-but-unwritten entries dropped.
    kh_cloth_workers_stop();   // Joined; its queues dropped.
    release_shadow_device_state();
    if (khst_clean) kh_mesh_release_session();   // Ahead of g_res.release, which frees the grave it fills.
    g_res.release();
    kh_cloth_drop_all();   // KH_CLOTH: the instances and their buffers.
    kh_skin_drop_all();    // KH_SKEL: likewise.
    kh_uvs_release_all();  // KH_USER_VS: likewise (the records hold nothing a mission outlives).
    kh_tex_cache_release();
    kh_user_shader_cache_release();
    kh_user_lut_cache_release();
    if (khst_clean) kh_material_pool_reset();
    RenderAssetDiscovery::clear_cache();
    kh_shader_mt_shutdown();   // No worker may outlive a mission.
    g_vs_memo_epoch.fetch_add(1u, std::memory_order_relaxed);   // Every thread's read memo empties at its next read.
    if (khst_clean) {
        kh_session_objects_reset();
    } else {
        // KH_ATTACH_CYCLE_RESTAMP: the next mission's bindings and chain ends stay (kh_session_objects_reset is the
        // clean edge's), but reset_session_state below restarts g_topo_cycles - a kept stamp would match once, when
        // the new counter reached it, and hand that cycle a pick, a read or a stash from months before (the rule the
        // scratch reset keeps for the global latches). Under the park, as the step that writes them runs.
        std::lock_guard<std::mutex> khst_g(g_draw_list_mutex);
        for (auto& khst_e : g_attach) {
            khst_e.second.gts_cyc = ~0ull;
            khst_e.second.rt_cyc = ~0ull;
            khst_e.second.rt_chk_cyc = ~0ull;
            khst_e.second.skel_smp_cycle = ~0ull;
            khst_e.second.smp_cycle = ~0ull;
            khst_e.second.skin_culled = false;   // KH_SKIN_CULL: the next step decides afresh.
            khst_e.second.cull_rest.reset();
            khst_e.second.cull_a.clear();
        }
        g_skin_cull_any = false;
        for (auto& khst_c : g_chain_end) khst_c.second.rt_cyc = ~0ull;
    }
    reset_session_state();
}

// Both mission edges: the event-handler handles and the per-mission retained
// state go first.
inline void kh_mission_handles_reset() {
    g_draw3d_eh_active = false;
    g_draw3d_eh = {};
    g_ui_driver_registered = false;
    g_ui_ctrl_created = false;
    g_ui_poll_eh = {};
    reset_retained_state();
}

// The whole-system reset, both mission edges: the handles and the retained
// state, then kh_session_teardown under the park (the prewarm refuses while any
// detached shader worker lives).
inline void rendering_integration_reset() {
    if (!kh_render_on()) return;   // KH_PLAYER_ONLY: nothing of this header ever started.
    bool khme_done = false;
    kh_mission_handles_reset();

    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_kh_track_wanted.store(false, std::memory_order_relaxed);

    for (int attempt = 0; attempt < 8; ++attempt) {
        RVExtBridge::ScopedGraphicsLock lock;

        if (!lock.acquired()) continue;
        kh_session_teardown(true);   // Clean: kh_mission_handles_reset emptied the draw list above.
        khme_done = true;
        break;
    }

    // A mission that never uses the extension holds the state disarmed.
    if (!khme_done) {
        // KH_TEARDOWN_KEEP_SETTINGS: the deferred teardown keeps the script settings it finds (the next
        // mission's), so this mission's go back to their declared defaults here, on the game thread (atomics).
        // The stats take reset_stat_counters: disarmed and zeroed, the unlocked best effort
        // reset_render_stats_sqf also falls back to.
        g_dls_allowed.store(false, std::memory_order_relaxed);
        g_khsg_scale.store(0.5f, std::memory_order_relaxed);
        g_ao_strength_bits.store(0x3F800000u, std::memory_order_relaxed);
        g_ao_dist_bits.store(0x3F000000u, std::memory_order_relaxed);
        reset_stat_counters();
        g_mission_destroy_pending.store(true, std::memory_order_relaxed);
    }
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
}

// Process detach (DllMain, the FreeLibrary path only - lpReserved == nullptr,
// like every other framework's teardown). On the terminating path the OS has
// already killed our threads and nothing here may run: the five thread
// objects (loader, cache writer, shader pool, cloth pool, skin pool) are
// heap-leaked so
// no static destructor can std::terminate on a still-joinable thread, which is
// what makes that path safe without a call.
//
// Unloading: our threads are alive and would return into unmapped code once
// the DLL goes. Each is asked to stop, waited for briefly and terminated -
// the shader pool's workers an earlier shutdown detached too, by the handles
// kept for them (g_khsm_orphans). Under
// the loader lock a worker that obeys cannot finish exiting (its thread detach
// needs that lock), so every wait runs its budget out and every worker is
// terminated; the stop request is what gets it out of our code first (a job
// inside D3DCompile or a large cache write cannot be interrupted). The
// framework's watchdog bounds the whole detach regardless.
// No graphics lock (the render thread may never park again) and no device
// release (a leaked COM reference at unload costs nothing; a context call
// could hang).
inline void kh_thread_stop_bounded(std::thread& khts_t, DWORD khts_ms) {
    if (!khts_t.joinable()) return;
    HANDLE khts_h = static_cast<HANDLE>(khts_t.native_handle());
    if (khts_h && WaitForSingleObject(khts_h, khts_ms) == WAIT_TIMEOUT) TerminateThread(khts_h, 0);
    try { khts_t.join(); } catch (...) { try { khts_t.detach(); } catch (...) {} }
}

// True when anything of the render integration is running or exists: script
// objects (meshes, post FX, live or staged), the per-frame drivers, an
// installed hook or the reset hook, device resources, a shader compile in any
// form (the pool, the async batch, a user .hlsl request), a worker thread or
// its queued / finished jobs, a pending mesh publish or deferred destroy, the
// caches, the shadow and PIP state, the terrain heightfield. A session that
// never touched the extension reads false on every term and DllMain skips the
// detach, as it does for the other frameworks. No lock may be taken from
// DllMain, so every read is lock-free: the script-side containers' writer is
// the game thread, the caller; the worker queues (g_khtl_q / g_khtl_done,
// g_khmw_q, g_khsm_map) and g_tex_cache have other writers and are read
// racily. Each worker's has a term that reads true while that writer can run
// (g_khtl_running, g_khmw_running, g_khsm_live); the render thread's writes
// (g_tex_cache, g_khsm_map's erases - the hooks' draws or the shared Present's
// UI flush) need an object or a UI effect, so g_draw_list / g_res.initialized
// read true. The verdict does not hang on the racy reads.
inline bool rendering_integration_is_initialized() {
    return !g_draw_list.empty() || g_scene.alive_n > 0 || !g_affectors.empty() || !g_cloth_proxy.empty() ||
           g_draw3d_eh_active || g_ui_driver_registered ||
           g_reorder_hook_active.load(std::memory_order_acquire) || g_reset_hook_installed ||
           g_res.initialized ||
           g_khsm_active || g_khsa_state != 0 || !g_khsm_thr.empty() ||
           g_khsm_live.load(std::memory_order_acquire) > 0 || !g_khsm_map.empty() ||
           g_user_batch_own || !g_user_req.empty() ||
           g_khtl_running || !g_khtl_q.empty() || !g_khtl_done.empty() ||
           g_khmw_running || !g_khmw_q.empty() ||
           !g_cloth_thr.empty() ||
           g_mesh_publish_pending.load(std::memory_order_relaxed) ||
           g_mission_destroy_pending.load(std::memory_order_relaxed) ||
           !g_tex_cache.empty() || !g_user_ps_cache.empty() || !g_user_lut_cache.empty() ||
           !g_mat_pool.empty() ||
           g_ls.wanted.load(std::memory_order_relaxed) || g_ls.atlas_tex != nullptr ||
           g_dlf_wanted.load(std::memory_order_relaxed) ||   // KH_DLF_DEMAND.
           g_pip_seen || g_thm_valid || g_thm_dirty || g_thm_auto_state != 0 ||
           g_stats_armed.load(std::memory_order_relaxed);
}

inline void rendering_integration_process_detach() {
    // Every hook is a pass-through from here and every demand gate reads off.
    g_reorder_target_ctx.store(nullptr, std::memory_order_relaxed);
    g_kh_track_wanted.store(false, std::memory_order_relaxed);
    g_ls.wanted.store(false, std::memory_order_relaxed);
    g_dlf_wanted.store(false, std::memory_order_relaxed);   // KH_DLF_DEMAND.
    g_dl_idle.store(0, std::memory_order_relaxed);   // KH_DL_IDLE.
    g_khsa_abort.store(true, std::memory_order_relaxed);
    g_khmw_abort.store(true, std::memory_order_relaxed);
    g_khtl_abort.store(true, std::memory_order_relaxed);
    g_cloth_stop.store(true, std::memory_order_relaxed);
    // The reset hook is a pointer into this DLL held by kh_rv_extension, a
    // module the engine loaded on its own and which outlives an unload of
    // ours; cleared so a later device reset cannot call into unmapped code.
    // The setter forwards a bare function pointer, so null is a plain store.
    if (g_reset_hook_installed) {
        RVExtBridge::set_reset_hook(nullptr);
        g_reset_hook_installed = false;
    }
    g_khtl_cv.notify_all();
    g_khmw_cv.notify_all();
    g_khsm_cv.notify_all();
    g_cloth_cv.notify_all();
    g_skin_cv.notify_all();   // KH_POOL_SPLIT.
    kh_thread_stop_bounded(g_khtl_thr, 200);
    kh_thread_stop_bounded(g_khmw_thr, 200);
    // The running flags without their mutexes: a worker terminated inside its
    // critical section leaves the mutex locked forever, and nothing starts a
    // worker after this point.
    g_khtl_running = false;
    g_khmw_running = false;
    // The pool: one budget for every worker, not one per worker.
    const uint64_t khpd_t0 = steady_now_ms();
    for (std::thread& khpd_t : g_khsm_thr) {
        const uint64_t khpd_el = steady_now_ms() - khpd_t0;
        kh_thread_stop_bounded(khpd_t, khpd_el < 200u ? static_cast<DWORD>(200u - khpd_el) : 0u);
    }
    g_khsm_thr.clear();
    // The workers an earlier shutdown detached, on what is left of the same budget (one inside D3DCompile cannot be
    // told to stop).
    for (const HANDLE khpd_h : g_khsm_orphans) {
        const uint64_t khpd_el = steady_now_ms() - khpd_t0;
        if (WaitForSingleObject(khpd_h, khpd_el < 200u ? static_cast<DWORD>(200u - khpd_el) : 0u) == WAIT_TIMEOUT) {
            TerminateThread(khpd_h, 0);
        }
        CloseHandle(khpd_h);
    }
    g_khsm_orphans.clear();
    g_khsm_active = false;
    g_khsa_state = 0;
    // The cloth pool, the same way. A worker woken with the stop set drains the
    // queues it finds - one step per queued cloth, one skinning per queued
    // skeletal mesh (KH_SKEL) - and returns; nothing queues after this point.
    const uint64_t khpd_c0 = steady_now_ms();
    for (std::thread& khpd_t : g_cloth_thr) {
        const uint64_t khpd_el = steady_now_ms() - khpd_c0;
        kh_thread_stop_bounded(khpd_t, khpd_el < 200u ? static_cast<DWORD>(200u - khpd_el) : 0u);
    }
    g_cloth_thr.clear();
    g_cloth_thr_n.store(0u, std::memory_order_relaxed);   // KH_CLOTH_FORK.
    for (std::thread& khpd_t : g_skin_thr) {   // KH_POOL_SPLIT: the skin pool, on the same budget.
        const uint64_t khpd_el = steady_now_ms() - khpd_c0;
        kh_thread_stop_bounded(khpd_t, khpd_el < 200u ? static_cast<DWORD>(200u - khpd_el) : 0u);
    }
    g_skin_thr.clear();
    g_skin_thr_n.store(0u, std::memory_order_relaxed);
}

inline std::string add_render_object(const RenderObject& obj) {
    if (!kh_render_on()) return std::string();   // KH_PLAYER_ONLY.
    ensure_draw_eh();
    g_kh_track_wanted.store(true, std::memory_order_relaxed);
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    const std::string handle = make_render_uid();
    RenderObject& stored = g_draw_list[handle];
    stored = obj;
    stored.seq = ++g_next_seq;   // Creation order (fullscreen pass chaining).
    stored.birth_time = effect_time_seconds();
    stored.fx_t0 = effect_time_seconds_d();
    stored.slot = kh_scene_slot_alloc();   // KH_SCENE.
    g_scene_stage[stored.slot] = &stored;   // Node-stable in unordered_map.
    g_scene_handle[stored.slot] = handle;
    kh_scene_mark(stored.slot);
    return handle;
}

// The script side's property update. The staged copy carries the slot from the
// entry it was copied from; the entry may have been removed and the slot reused
// meanwhile, so the slot is re-taken from the live entry.
// KH_UPDATE_MERGE: staged was copied from the entry (base) and the property
// applied to it outside the lock, while other writers kept writing the entry:
// kh_attach_step (render thread too) the pose (pos, rot_m, rotated), the skin
// and chain uploads and the skinned draw the box (size, skel_ctr, pos), and
// the cloth upload a plain cloth's refitted bound (cast_size). Each of
// those groups is taken from the live entry unless the property changed it
// (staged differs from base; for the box, also unless the mesh or the box's
// owner changed; cast_size always, no property writes it) - the
// property wins what it set, and nothing it did not set is written back from
// the copy's older state.
inline bool update_render_object(const std::string& handle, RenderObject&& staged, const RenderObject& base) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_draw_list.find(handle);
    if (it == g_draw_list.end()) return false;
    const RenderObject& khuo_live = it->second;
    staged.slot = khuo_live.slot;
    if (memcmp(staged.pos, base.pos, sizeof(staged.pos)) == 0) {
        memcpy(staged.pos, khuo_live.pos, sizeof(staged.pos));
        memcpy(staged.pos_res, khuo_live.pos_res, sizeof(staged.pos_res));   // KH_POS_RES.
        memcpy(staged.pos_res_at, khuo_live.pos_res_at, sizeof(staged.pos_res_at));
    }
    if (memcmp(staged.rot_m, base.rot_m, sizeof(staged.rot_m)) == 0 && staged.rotated == base.rotated) {
        memcpy(staged.rot_m, khuo_live.rot_m, sizeof(staged.rot_m));
        staged.rotated = khuo_live.rotated;
    }
    // The box is its mesh's and its owner's (the skin while skel, the chain
    // while chain_sim, else the script): a property that changed either owns
    // it whole, and so does an entry another update re-owned meanwhile.
    if (staged.mesh == base.mesh && khuo_live.mesh == base.mesh &&
        staged.skel == base.skel && khuo_live.skel == base.skel &&
        staged.chain_sim == base.chain_sim && khuo_live.chain_sim == base.chain_sim) {
        if (memcmp(staged.size, base.size, sizeof(staged.size)) == 0) {
            memcpy(staged.size, khuo_live.size, sizeof(staged.size));
        }
        if (memcmp(staged.skel_ctr, base.skel_ctr, sizeof(staged.skel_ctr)) == 0) {
            memcpy(staged.skel_ctr, khuo_live.skel_ctr, sizeof(staged.skel_ctr));
        }
    }
    // KH_CAST_REFIT: the cloth upload's put (kh_cloth_upload, in flush_locked -
    // the render thread's in the KH_NO_PARK path) owns a plain cloth's
    // cast_size and no property writes it, so the live entry's is the
    // newest; the copy's is the snapshot's, a bound the reach may have
    // outgrown since (a frame of the smaller bound at the frustum or
    // shadow edge, until the next flush re-put it). It is a bound only
    // while the object is still what the put admits (a plain cloth: not
    // skeletal, not a chain) on the mesh it was measured on; a property
    // that ends that returns it to zero, and the bound is size again (the
    // put never clears it, and a new mesh's reach starts unmeasured).
    if (staged.mesh == base.mesh && !staged.skel && !staged.chain_sim && kh_cloth_obj_sim(staged)) {
        memcpy(staged.cast_size, khuo_live.cast_size, sizeof(staged.cast_size));
    } else {
        memset(staged.cast_size, 0, sizeof(staged.cast_size));
    }
    it->second = staged;
    kh_scene_mark(it->second.slot);
    return true;
}

inline bool remove_render_object(const std::string& handle) {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    auto it = g_draw_list.find(handle);
    if (it == g_draw_list.end()) return false;
    kh_scene_stage_remove(it->second);
    kh_attach_drop(handle);   // KH_ATTACH.
    g_draw_list.erase(it);
    return true;
}

inline size_t clear_render_objects() {
    std::lock_guard<std::mutex> g(g_draw_list_mutex);
    const size_t n = g_draw_list.size();
    for (auto& kv : g_draw_list) kh_scene_stage_remove(kv.second);
    kh_attach_drop_all(true);   // KH_ATTACH: mid-mission, so the proxies are ours to delete.
    g_draw_list.clear();
    return n;
}

// KH_ALL_HANDLES: every live handle, appended in creation order (seq). The map
// is unordered, so the walk collects under the mutex and sorts outside it.
inline void render_object_handles(std::vector<std::string>& khrh_out) {
    std::vector<std::pair<uint64_t, std::string>> khrh_v;
    {
        std::lock_guard<std::mutex> g(g_draw_list_mutex);
        khrh_v.reserve(g_draw_list.size());
        for (const auto& kv : g_draw_list) khrh_v.emplace_back(kv.second.seq, kv.first);
    }
    std::sort(khrh_v.begin(), khrh_v.end());
    for (auto& khrh_p : khrh_v) khrh_out.push_back(std::move(khrh_p.second));
}

}

#undef KH_SAFE_RELEASE