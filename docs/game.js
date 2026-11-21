// Mini TPS di prova per PigeonCrosshair. Three.js, nessuna build: viene caricato come modulo ES.
// Il mirino NON e' disegnato qui: e' il canvas 2D in index.html, fisso al centro dello schermo.
import * as THREE from "https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.module.js";

const G = window.PCG;                       // stato condiviso con index.html
const $ = id => document.getElementById(id);
const stage = $("stage"), canvas = $("gl"), startEl = $("start"), startMsg = $("startMsg"),
      killsEl = $("kills"), hpEl = $("hpfill"), hurtEl = $("hurt"), noGl = $("nogl");
const T = () => (window.pcT ? window.pcT() : {});

/* ---------- renderer / scena ---------- */
let renderer;
try {
  renderer = new THREE.WebGLRenderer({ canvas, antialias: true });
} catch (e) { noGl.hidden = false; throw e; }
renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;

const scene = new THREE.Scene();
scene.fog = new THREE.Fog(0x8f7089, 30, 95);
const camera = new THREE.PerspectiveCamera(60, 16 / 9, 0.1, 300);
camera.rotation.order = "YXZ";

// cielo a gradiente
scene.add(new THREE.Mesh(
  new THREE.SphereGeometry(200, 24, 16),
  new THREE.ShaderMaterial({
    side: THREE.BackSide, depthWrite: false, fog: false,
    uniforms: { top: { value: new THREE.Color(0x1d2540) }, mid: { value: new THREE.Color(0x58507a) }, bot: { value: new THREE.Color(0xe49a7c) } },
    vertexShader: "varying float h; void main(){ h = normalize(position).y; gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0); }",
    fragmentShader: "uniform vec3 top; uniform vec3 mid; uniform vec3 bot; varying float h; void main(){ vec3 c = h > 0.0 ? mix(mid, top, smoothstep(0.0, 0.7, h)) : mix(mid, bot, smoothstep(0.0, -0.25, h)); gl_FragColor = linearToOutputTexel(vec4(c, 1.0)); }"
  })
));
scene.add(new THREE.HemisphereLight(0xcfc8ee, 0x5a4a60, 1.6));
const sun = new THREE.DirectionalLight(0xffd3a6, 2.4);
sun.position.set(-22, 32, 16);
sun.castShadow = true;
sun.shadow.mapSize.set(1024, 1024);
Object.assign(sun.shadow.camera, { left: -42, right: 42, top: 42, bottom: -42, near: 1, far: 100 });
scene.add(sun);

/* ---------- mondo ---------- */
const ARENA = 34;
function gridTexture() {
  const c = document.createElement("canvas"); c.width = c.height = 128;
  const g = c.getContext("2d");
  g.fillStyle = "#3b4359"; g.fillRect(0, 0, 128, 128);
  g.strokeStyle = "#566082"; g.lineWidth = 2; g.strokeRect(0, 0, 128, 128);
  const t = new THREE.CanvasTexture(c);
  t.wrapS = t.wrapT = THREE.RepeatWrapping; t.repeat.set(40, 40);
  t.colorSpace = THREE.SRGBColorSpace; t.anisotropy = 4;
  return t;
}
const ground = new THREE.Mesh(new THREE.PlaneGeometry(240, 240), new THREE.MeshStandardMaterial({ map: gridTexture(), roughness: 1 }));
ground.rotation.x = -Math.PI / 2; ground.receiveShadow = true;
scene.add(ground);

const obstacles = [];          // {mesh,minX,maxX,minZ,maxZ}
const obstacleMeshes = [];
function addBox(x, z, w, d, h, color) {
  const m = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), new THREE.MeshStandardMaterial({ color, roughness: 0.9, flatShading: true }));
  m.position.set(x, h / 2, z); m.castShadow = m.receiveShadow = true;
  scene.add(m); obstacleMeshes.push(m);
  obstacles.push({ mesh: m, minX: x - w / 2, maxX: x + w / 2, minZ: z - d / 2, maxZ: z + d / 2 });
}
// pareti perimetrali
addBox(0, -ARENA - 1, ARENA * 2 + 4, 2, 4, 0x39405a); addBox(0, ARENA + 1, ARENA * 2 + 4, 2, 4, 0x39405a);
addBox(-ARENA - 1, 0, 2, ARENA * 2, 4, 0x39405a);      addBox(ARENA + 1, 0, 2, ARENA * 2, 4, 0x39405a);
// ripari (seed fisso: arena uguale per tutti)
(function () {
  let s = 1234567; const rnd = () => { s |= 0; s = (s + 0x6D2B79F5) | 0; let t = Math.imul(s ^ (s >>> 15), 1 | s); t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t; return ((t ^ (t >>> 14)) >>> 0) / 4294967296; };
  const cols = [0x5b4b6e, 0x6e5a5a, 0x4a5f73, 0x7a6a52, 0x4e6b5f];
  let placed = 0, tries = 0;
  while (placed < 16 && tries++ < 200) {
    const x = (rnd() * 2 - 1) * (ARENA - 4), z = (rnd() * 2 - 1) * (ARENA - 4);
    if (Math.hypot(x, z) < 8 || (Math.abs(x) < 7 && z < -4 && z > -22)) continue;   // spazio libero attorno al giocatore
    addBox(x, z, 2 + rnd() * 3.5, 2 + rnd() * 3.5, 1.4 + rnd() * 2.4, cols[placed % cols.length]); placed++;
  }
})();

function pushOut(p, r) {
  for (const o of obstacles) {
    const cx = Math.max(o.minX, Math.min(p.x, o.maxX)), cz = Math.max(o.minZ, Math.min(p.z, o.maxZ));
    const dx = p.x - cx, dz = p.z - cz, d2 = dx * dx + dz * dz;
    if (d2 >= r * r) continue;
    if (d2 > 1e-6) { const d = Math.sqrt(d2); p.x = cx + dx / d * r; p.z = cz + dz / d * r; }
    else {
      const l = p.x - o.minX, rr = o.maxX - p.x, t = p.z - o.minZ, b = o.maxZ - p.z, m = Math.min(l, rr, t, b);
      if (m === l) p.x = o.minX - r; else if (m === rr) p.x = o.maxX + r; else if (m === t) p.z = o.minZ - r; else p.z = o.maxZ + r;
    }
  }
}

/* ---------- personaggi ---------- */
function humanoid(shirt, skin = 0xe2b592, pants = 0x262b38) {
  const g = new THREE.Group();
  const mat = c => new THREE.MeshStandardMaterial({ color: c, roughness: 0.85, flatShading: true });
  const part = (geo, c, x, y, z, parent = g) => { const m = new THREE.Mesh(geo, mat(c)); m.position.set(x, y, z); m.castShadow = true; parent.add(m); return m; };
  part(new THREE.BoxGeometry(0.58, 0.72, 0.32), shirt, 0, 1.12, 0);
  const head = part(new THREE.SphereGeometry(0.2, 12, 10), skin, 0, 1.68, 0); head.name = "head";
  const pivot = (x, y) => { const p = new THREE.Group(); p.position.set(x, y, 0); g.add(p); return p; };
  const legL = pivot(-0.15, 0.76), legR = pivot(0.15, 0.76), armL = pivot(-0.4, 1.42), armR = pivot(0.4, 1.42);
  part(new THREE.BoxGeometry(0.22, 0.74, 0.24), pants, 0, -0.37, 0, legL);
  part(new THREE.BoxGeometry(0.22, 0.74, 0.24), pants, 0, -0.37, 0, legR);
  part(new THREE.BoxGeometry(0.17, 0.62, 0.2), shirt, 0, -0.29, 0, armL);
  part(new THREE.BoxGeometry(0.17, 0.62, 0.2), shirt, 0, -0.29, 0, armR);
  g.userData = { legL, legR, armL, armR };
  return g;
}

const player = { g: humanoid(0x4f7cff), vy: 0, hp: 100, walk: 0 };
const gun = new THREE.Mesh(new THREE.BoxGeometry(0.09, 0.13, 0.55), new THREE.MeshStandardMaterial({ color: 0x15181f, roughness: 0.5 }));
gun.position.set(0.3, 1.3, -0.45); gun.castShadow = true;
player.g.add(gun);
const muzzle = new THREE.Object3D(); muzzle.position.set(0, 0, -0.32); gun.add(muzzle);
scene.add(player.g);

const BOT_COLORS = [0xd9534f, 0xe0a030, 0x45a889, 0xb05fd0, 0xd0d0d8];
let bots = [];
function spawnBot(x, z) {
  const group = humanoid(BOT_COLORS[(Math.random() * BOT_COLORS.length) | 0]);
  group.position.set(x, 0, z);
  const bot = { g: group, hp: 100, dead: false, t: 0, hitFlash: 0, atk: 0, walk: Math.random() * 6, speed: 1.9 + Math.random() * 0.8 };
  group.traverse(o => { if (o.isMesh) o.userData.bot = bot; });
  scene.add(group); bots.push(bot);
  return bot;
}
function randomSpawn() {
  for (let i = 0; i < 40; i++) {
    const a = Math.random() * Math.PI * 2, r = 16 + Math.random() * 15;
    const p = { x: player.g.position.x + Math.cos(a) * r, z: player.g.position.z + Math.sin(a) * r };
    if (Math.abs(p.x) > ARENA - 2 || Math.abs(p.z) > ARENA - 2) continue;
    const q = { x: p.x, z: p.z }; pushOut(q, 1);
    if (Math.hypot(q.x - p.x, q.z - p.z) < 0.01) return p;
  }
  return { x: 0, z: -20 };
}

/* ---------- stato di gioco ---------- */
const state = { locked: false, over: false, needsReset: true };
let yaw = 0, pitch = 0.05, rec = 0, kills = 0, spawnT = 0, fireT = 0, firing = false;
const keys = {};
const fx = [];                 // effetti brevi (tracciante, lampo, scintille)

function clearBots() { bots.forEach(b => scene.remove(b.g)); bots = []; }
function idleScene() {
  clearBots();
  [[-5, -10], [4, -13], [0, -19]].forEach(([x, z]) => { const b = spawnBot(x, z); b.g.rotation.y = Math.PI; });
}
function setHp(v) { player.hp = Math.max(0, v); G.hp = player.hp; hpEl.style.width = player.hp + "%"; }
function setKills(v) { kills = v; G.kills = v; killsEl.textContent = v; }
function reset() {
  player.g.position.set(0, 0, 0); player.vy = 0; yaw = 0; pitch = 0.05; rec = 0;
  setHp(100); setKills(0); clearBots(); spawnT = 0.4; state.over = false; state.needsReset = false;
}
function showStart(kind) {
  const t = T();
  startMsg.textContent = kind === "over" ? (t.over || "").replace("{n}", kills) : kind === "resume" ? t.resume : t.play;
  startEl.hidden = false;
}
idleScene(); showStart("play");

/* ---------- input ---------- */
const coarse = matchMedia("(pointer: coarse)").matches || !("onpointerlockchange" in document);
if (coarse) { startMsg.textContent = (T().desktop || "Serve un computer con mouse e tastiera."); stage.dataset.nolock = "1"; }

stage.addEventListener("click", () => { if (!state.locked && !stage.dataset.nolock) stage.requestPointerLock?.(); });
document.addEventListener("pointerlockchange", () => {
  state.locked = document.pointerLockElement === stage;
  G.playing = state.locked;
  if (state.locked) { if (state.needsReset) reset(); startEl.hidden = true; }
  else { firing = false; if (!state.over) showStart(state.needsReset ? "play" : "resume"); }
});
document.addEventListener("mousemove", e => {
  if (!state.locked) return;
  const k = 0.0022 * G.sens;
  yaw -= e.movementX * k;
  pitch = Math.max(-1.15, Math.min(1.25, pitch - e.movementY * k));
});
window.addEventListener("mousedown", e => { if (state.locked && e.button === 0) { firing = true; } });
window.addEventListener("mouseup", e => { if (e.button === 0) firing = false; });
window.addEventListener("keydown", e => {
  keys[e.code] = true;
  if (state.locked && ["Space", "ArrowUp", "ArrowDown"].includes(e.code)) e.preventDefault();
});
window.addEventListener("keyup", e => { keys[e.code] = false; });
window.addEventListener("blur", () => { for (const k in keys) keys[k] = false; firing = false; });
window.pcOnLang = () => { if (!startEl.hidden) showStart(state.over ? "over" : state.needsReset ? "play" : "resume"); };

/* ---------- audio ---------- */
let ac;
function beep(f, d, type, vol) {
  try {
    ac = ac || new AudioContext(); const o = ac.createOscillator(), g = ac.createGain(), t = ac.currentTime;
    o.type = type; o.frequency.setValueAtTime(f, t); o.frequency.exponentialRampToValueAtTime(40, t + d);
    g.gain.setValueAtTime(vol, t); g.gain.exponentialRampToValueAtTime(0.001, t + d);
    o.connect(g).connect(ac.destination); o.start(); o.stop(t + d);
  } catch (e) { /* audio non disponibile */ }
}

/* ---------- sparo ---------- */
const ray = new THREE.Raycaster();
const tmp = new THREE.Vector3();
function addFx(mesh, life) { scene.add(mesh); fx.push({ mesh, life, max: life }); }
const flashMat = new THREE.MeshBasicMaterial({ color: 0xffe9a0, transparent: true });
function shoot() {
  beep(240, 0.09, "sawtooth", 0.07);
  rec = Math.min(rec + 0.012, 0.09);
  ray.setFromCamera({ x: 0, y: 0 }, camera);            // centro dello schermo = dove sta il mirino
  ray.far = 150;
  const alive = bots.filter(b => !b.dead).map(b => b.g);
  const hits = ray.intersectObjects([...alive, ...obstacleMeshes, ground], true);
  const hit = hits[0];
  const end = hit ? hit.point.clone() : ray.ray.at(120, new THREE.Vector3());
  // tracciante
  gun.updateWorldMatrix(true, false);
  const from = muzzle.getWorldPosition(new THREE.Vector3());
  const len = from.distanceTo(end);
  const tr = new THREE.Mesh(new THREE.BoxGeometry(0.025, 0.025, 1), new THREE.MeshBasicMaterial({ color: 0xfff0b0, transparent: true }));
  tr.position.copy(from).lerp(end, 0.5); tr.scale.z = len; tr.lookAt(end);
  addFx(tr, 0.07);
  const fl = new THREE.Mesh(new THREE.SphereGeometry(0.11, 8, 6), flashMat.clone());
  fl.position.copy(from); addFx(fl, 0.05);
  if (!hit) return;
  let o = hit.object, bot = null;
  while (o && !bot) { bot = o.userData.bot; o = o.parent; }
  if (bot && !bot.dead) {
    const head = hit.object.name === "head";
    bot.hp -= head ? 100 : 25; bot.hitFlash = 0.09;
    G.hitUntil = performance.now() + 140; G.hitHead = head;
    if (bot.hp <= 0) {
      bot.dead = true; bot.t = 0; setKills(kills + 1); G.killUntil = performance.now() + 260;
      beep(520, 0.14, "triangle", 0.08);
    } else beep(900, 0.05, "square", 0.05);
  } else {
    const sp = new THREE.Mesh(new THREE.BoxGeometry(0.12, 0.12, 0.12), new THREE.MeshBasicMaterial({ color: 0xffd9a0, transparent: true }));
    sp.position.copy(end); addFx(sp, 0.18);
  }
}

/* ---------- loop ---------- */
function resize() {
  const w = Math.max(2, stage.clientWidth), h = Math.max(2, stage.clientHeight);
  renderer.setSize(w, h, false); camera.aspect = w / h; applyFov();
}
let lastFov = 0;
function applyFov() {
  // FOV orizzontale (convenzione dei giochi), convertito nel verticale richiesto da three.js
  const v = THREE.MathUtils.radToDeg(2 * Math.atan(Math.tan(THREE.MathUtils.degToRad(G.fov) / 2) / camera.aspect));
  camera.fov = v; camera.updateProjectionMatrix(); lastFov = G.fov;
}
new ResizeObserver(resize).observe(stage); resize();

const clock = new THREE.Clock();
function animate() {
  requestAnimationFrame(animate);
  const dt = Math.min(clock.getDelta(), 0.05), now = performance.now();
  if (G.fov !== lastFov) applyFov();
  const live = state.locked && !state.over;
  const pos = player.g.position;

  if (live) {
    // movimento relativo alla visuale
    let f = (keys.KeyW ? 1 : 0) - (keys.KeyS ? 1 : 0), s = (keys.KeyD ? 1 : 0) - (keys.KeyA ? 1 : 0);
    const l = Math.hypot(f, s) || 1; f /= l; s /= l;
    const sp = (keys.ShiftLeft || keys.ShiftRight) ? 7.4 : 5;
    pos.x += (-Math.sin(yaw) * f + Math.cos(yaw) * s) * sp * dt;
    pos.z += (-Math.cos(yaw) * f - Math.sin(yaw) * s) * sp * dt;
    pos.x = Math.max(-ARENA, Math.min(ARENA, pos.x)); pos.z = Math.max(-ARENA, Math.min(ARENA, pos.z));
    pushOut(pos, 0.4);
    if (keys.Space && pos.y <= 0.001) player.vy = 6.2;
    player.vy -= 18 * dt; pos.y = Math.max(0, pos.y + player.vy * dt); if (pos.y === 0) player.vy = 0;
    player.walk += (f || s ? sp : 0) * dt * 1.7;
    // sparo automatico
    fireT -= dt;
    if (firing && fireT <= 0) { shoot(); fireT = 0.11; }
    // spawn
    const maxAlive = Math.min(9, 4 + Math.floor(kills / 5));
    spawnT -= dt;
    if (spawnT <= 0 && bots.filter(b => !b.dead).length < maxAlive) { const p = randomSpawn(); spawnBot(p.x, p.z); spawnT = 1.1; }
    rec = Math.max(0, rec - dt * 0.12);
  }
  player.g.rotation.y = yaw;
  const swing = Math.sin(player.walk) * 0.7 * (live && (keys.KeyW || keys.KeyA || keys.KeyS || keys.KeyD) ? 1 : 0);
  const u = player.g.userData;
  u.legL.rotation.x = swing; u.legR.rotation.x = -swing; u.armL.rotation.x = -swing; u.armR.rotation.x = -1.45 + (pitch + rec) * 0.9;
  gun.rotation.x = (pitch + rec) * 0.9; gun.position.y = 1.3 + (pitch + rec) * 0.08;

  // bot
  for (const b of bots) {
    const bu = b.g.userData;
    if (b.dead) {
      b.t += dt; b.g.rotation.x = Math.min(Math.PI / 2, b.g.rotation.x + dt * 5); b.g.position.y = Math.max(-0.5, b.g.position.y - (b.t > 0.9 ? dt * 0.8 : 0));
      continue;
    }
    if (live) {
      const dx = pos.x - b.g.position.x, dz = pos.z - b.g.position.z, d = Math.hypot(dx, dz) || 1;
      b.g.rotation.y = Math.atan2(-dx, -dz);
      if (d > 1.5) {
        let mx = dx / d, mz = dz / d;
        for (const o of bots) { if (o === b || o.dead) continue; const ox = b.g.position.x - o.g.position.x, oz = b.g.position.z - o.g.position.z, od = Math.hypot(ox, oz); if (od < 1.2 && od > 0.01) { mx += ox / od * 0.8; mz += oz / od * 0.8; } }
        const ml = Math.hypot(mx, mz) || 1, sp = b.speed + Math.min(1, kills / 40);
        b.g.position.x += mx / ml * sp * dt; b.g.position.z += mz / ml * sp * dt; pushOut(b.g.position, 0.4); b.walk += dt * 6;
      } else {
        b.atk -= dt;
        if (b.atk <= 0) { b.atk = 0.8; setHp(player.hp - 8); hurtEl.style.opacity = 0.65; beep(120, 0.1, "square", 0.06); }
      }
      bu.legL.rotation.x = Math.sin(b.walk) * 0.7; bu.legR.rotation.x = -Math.sin(b.walk) * 0.7;
      bu.armL.rotation.x = -Math.sin(b.walk) * 0.7; bu.armR.rotation.x = d <= 1.7 ? -1.3 : Math.sin(b.walk) * 0.7;
    }
    if (b.hitFlash > 0) { b.hitFlash -= dt; b.g.traverse(o => { if (o.isMesh && o.material.emissive) o.material.emissive.setScalar(b.hitFlash > 0 ? 0.7 : 0); }); }
  }
  bots = bots.filter(b => { if (b.dead && b.t > 1.7) { scene.remove(b.g); return false; } return true; });

  if (live && player.hp <= 0) {
    state.over = true; state.needsReset = true; G.playing = false; document.exitPointerLock?.(); showStart("over");
  }
  hurtEl.style.opacity = Math.max(0, parseFloat(hurtEl.style.opacity || 0) - dt * 1.4);

  // effetti
  for (let i = fx.length - 1; i >= 0; i--) {
    const e = fx[i]; e.life -= dt; const k = Math.max(0, e.life / e.max);
    e.mesh.material.opacity = k; if (e.life <= 0) { scene.remove(e.mesh); e.mesh.geometry.dispose(); e.mesh.material.dispose(); fx.splice(i, 1); }
  }

  // camera in terza persona (spalla destra), con collisione contro i ripari
  const p = pitch + rec;
  const head = tmp.set(pos.x, pos.y + 1.55, pos.z).clone();
  const aim = new THREE.Vector3(-Math.sin(yaw) * Math.cos(p), Math.sin(p), -Math.cos(yaw) * Math.cos(p));
  const right = new THREE.Vector3(Math.cos(yaw), 0, -Math.sin(yaw));
  const want = head.clone().addScaledVector(aim, -4.2).addScaledVector(right, 0.85); want.y += 0.25;
  const dir = want.clone().sub(head), dist = dir.length(); dir.normalize();
  ray.set(head, dir); ray.far = dist;
  const ch = ray.intersectObjects(obstacleMeshes, false)[0];
  if (ch) want.copy(head).addScaledVector(dir, Math.max(0.4, ch.distance - 0.3));
  if (want.y < 0.3) want.y = 0.3;
  camera.position.copy(want); camera.rotation.set(p, yaw, 0);

  renderer.render(scene, camera);
}
animate();
