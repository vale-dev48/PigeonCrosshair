// Mini TPS di prova per PigeonCrosshair. Three.js, nessuna build: viene caricato come modulo ES.
// Il mirino NON e' disegnato qui: e' il canvas 2D in index.html, fisso al centro dello schermo.
import * as THREE from "https://cdn.jsdelivr.net/npm/three@0.160.0/build/three.module.js";

const G = window.PCG;                       // stato condiviso con index.html
const $ = id => document.getElementById(id);
const stage = $("stage"), canvas = $("gl"), startEl = $("start"), startMsg = $("startMsg"),
      killsEl = $("kills"), hurtEl = $("hurt"), noGl = $("nogl"), dmgsEl = $("dmgs"),
      shFill = $("shfill"), hpFill = $("hpfill"), shNum = $("shnum"), hpNum = $("hpnum"),
      wEls = [$("w0"), $("w1")];
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

/* ---------- armi ---------- */
const WEAPONS = [
  { id: "ar", rate: 0.1,  pellets: 1, dmg: 19, head: 1.8, spread: 0.006, rec: 0.011, zoom: 0.28, sfx: "ar" },
  { id: "sg", rate: 0.85, pellets: 8, dmg: 14, head: 1.5, spread: 0.04,  rec: 0.06,  zoom: 0.18, sfx: "sg", falloff: true }
];
const darkMat = new THREE.MeshStandardMaterial({ color: 0x15181f, roughness: 0.5 });
const woodMat = new THREE.MeshStandardMaterial({ color: 0x6b4a2f, roughness: 0.7 });
function gunMesh(parts, muzzleZ) {
  const g = new THREE.Group();
  for (const [w, h, d, x, y, z, m] of parts) { const b = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), m); b.position.set(x, y, z); b.castShadow = true; g.add(b); }
  const mz = new THREE.Object3D(); mz.position.set(0, 0, muzzleZ); g.add(mz); g.userData.muzzle = mz;
  g.position.set(0.3, 1.3, -0.45);
  return g;
}
const guns = [
  gunMesh([[0.08, 0.12, 0.62, 0, 0, 0, darkMat], [0.07, 0.07, 0.3, 0, 0, -0.42, darkMat], [0.06, 0.2, 0.08, 0, -0.14, 0.05, darkMat]], -0.62),
  gunMesh([[0.1, 0.1, 0.55, 0, 0, 0, woodMat], [0.07, 0.07, 0.5, 0, 0.02, -0.4, darkMat], [0.13, 0.09, 0.2, 0, -0.03, -0.15, darkMat]], -0.68)
];
let cur = 0;

const player = { g: humanoid(0x4f7cff), vy: 0, hp: 100, sh: 100, walk: 0, crouch: 0 };
guns.forEach(g => player.g.add(g));
scene.add(player.g);
const muzzle = () => guns[cur].userData.muzzle;

/* ---------- barre sopra i bot ---------- */
const barGeo = new THREE.PlaneGeometry(1, 1); barGeo.translate(0.5, 0, 0);
function makeBars() {
  const g = new THREE.Group();
  const mk = (color, x, y, w, h, op, order) => {
    const m = new THREE.Mesh(barGeo, new THREE.MeshBasicMaterial({ color, transparent: true, opacity: op, depthTest: false, depthWrite: false }));
    m.position.set(x, y, 0); m.scale.set(w, h, 1); m.renderOrder = order; g.add(m); return m;
  };
  mk(0x000000, -0.54, 0.0, 1.08, 0.19, 0.6, 19);
  const sh = mk(0x3fa9ff, -0.5, 0.04, 1, 0.065, 1, 20), hp = mk(0x4fe08a, -0.5, -0.04, 1, 0.065, 1, 20);
  g.userData = { sh, hp };
  scene.add(g);
  return g;
}

const BOT_COLORS = [0xd9534f, 0xe0a030, 0x45a889, 0xb05fd0, 0xd0d0d8];
let bots = [];
function spawnBot(x, z) {
  const group = humanoid(BOT_COLORS[(Math.random() * BOT_COLORS.length) | 0]);
  group.position.set(x, 0, z);
  const bot = {
    g: group, sh: 100, hp: 100, dead: false, t: 0, hitFlash: 0, walk: Math.random() * 6,
    speed: 2.0 + Math.random() * 0.7, pref: 9 + Math.random() * 8, strafe: Math.random() < 0.5 ? 1 : -1, strafeT: 1 + Math.random() * 2,
    cool: 1 + Math.random() * 1.5, burst: 0, shotT: 0, los: false, losT: Math.random() * 0.25, bars: makeBars()
  };
  group.traverse(o => { if (o.isMesh) o.userData.bot = bot; });
  scene.add(group); bots.push(bot);
  updateBotBars(bot);
  return bot;
}
function updateBotBars(b) {
  b.bars.userData.sh.scale.x = Math.max(0.0001, b.sh / 100);
  b.bars.userData.hp.scale.x = Math.max(0.0001, b.hp / 100);
}
function removeBot(b) { scene.remove(b.g); scene.remove(b.bars); }
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
let yaw = 0, pitch = 0.05, rec = 0, kills = 0, spawnT = 0, fireT = 0, firing = false, aiming = false, aimT = 0;
const keys = {};
const fx = [];                 // effetti brevi (tracciante, lampo, scintille)

function setWeapon(i) {
  cur = i; guns.forEach((g, k) => { g.visible = k === i; });
  wEls.forEach((el, k) => el && el.classList.toggle("on", k === i));
  fireT = Math.max(fireT, 0.22);
}
function clearBots() { bots.forEach(removeBot); bots = []; }
function idleScene() {
  clearBots();
  [[-5, -10], [4, -13], [0, -19]].forEach(([x, z]) => { const b = spawnBot(x, z); b.g.rotation.y = Math.PI; });
}
function updateVitals() {
  shFill.style.width = player.sh + "%"; hpFill.style.width = Math.max(0, player.hp) + "%";
  shNum.textContent = Math.ceil(player.sh); hpNum.textContent = Math.ceil(Math.max(0, player.hp));
  G.hp = player.hp;
}
function setKills(v) { kills = v; G.kills = v; killsEl.textContent = v; }
function hurtPlayer(n) {
  const s = Math.min(player.sh, n); player.sh -= s; player.hp -= n - s;
  hurtEl.style.opacity = Math.min(0.85, 0.4 + n * 0.03);
  updateVitals(); sfx("hurt");
}
function reset() {
  player.g.position.set(0, 0, 0); player.vy = 0; player.crouch = 0; yaw = 0; pitch = 0.05; rec = 0; aimT = 0; aiming = false;
  player.sh = 100; player.hp = 100; updateVitals(); setKills(0); clearBots();
  spawnT = 0.4; state.over = false; state.needsReset = false; setWeapon(0);
}
function showStart(kind) {
  const t = T();
  startMsg.textContent = kind === "over" ? (t.over || "").replace("{n}", kills) : kind === "resume" ? t.resume : t.play;
  startEl.hidden = false;
}

/* ---------- input ---------- */
const coarse = matchMedia("(pointer: coarse)").matches || !("onpointerlockchange" in document);

stage.addEventListener("click", () => { audioInit(); if (!state.locked && !stage.dataset.nolock) stage.requestPointerLock?.(); });
stage.addEventListener("contextmenu", e => e.preventDefault());             // tasto destro = mira, niente menu
document.addEventListener("pointerlockchange", () => {
  state.locked = document.pointerLockElement === stage;
  G.playing = state.locked;
  if (state.locked) { if (state.needsReset) reset(); startEl.hidden = true; }
  else { firing = false; aiming = false; if (!state.over) showStart(state.needsReset ? "play" : "resume"); }
});
document.addEventListener("mousemove", e => {
  if (!state.locked) return;
  const k = 0.0022 * G.sens * (1 - 0.4 * aimT);                              // mirando la sensibilita' scende
  yaw -= e.movementX * k;
  pitch = Math.max(-1.15, Math.min(1.25, pitch - e.movementY * k));
});
window.addEventListener("mousedown", e => {
  if (!state.locked) return;
  if (e.button === 0) firing = true;
  if (e.button === 2) aiming = true;
});
window.addEventListener("mouseup", e => {
  if (e.button === 0) firing = false;
  if (e.button === 2) aiming = false;
});
window.addEventListener("wheel", e => { if (state.locked && Math.abs(e.deltaY) > 4) setWeapon((cur + 1) % WEAPONS.length); }, { passive: true });
window.addEventListener("keydown", e => {
  keys[e.code] = true;
  if (!state.locked) return;
  if (e.code === "Digit1") setWeapon(0);
  if (e.code === "Digit2") setWeapon(1);
  if (["Space", "ArrowUp", "ArrowDown"].includes(e.code)) e.preventDefault();
});
window.addEventListener("keyup", e => { keys[e.code] = false; });
window.addEventListener("blur", () => { for (const k in keys) keys[k] = false; firing = false; aiming = false; });
window.pcOnLang = () => { if (!startEl.hidden) showStart(state.over ? "over" : state.needsReset ? "play" : "resume"); };

/* ---------- audio ---------- */
// Un solo AudioContext, un'uscita comune con compressore (niente clipping quando i suoni si sovrappongono),
// attacco di 4 ms e rilascio morbido su ogni suono (niente "click" all'inizio e alla fine).
let ac = null, master = null, noiseBuf = null;
function audioInit() {
  try {
    if (!ac) {
      ac = new (window.AudioContext || window.webkitAudioContext)();
      const comp = ac.createDynamicsCompressor();
      comp.threshold.value = -18; comp.knee.value = 12; comp.ratio.value = 6; comp.attack.value = 0.003; comp.release.value = 0.12;
      master = ac.createGain(); master.gain.value = 0.55;
      master.connect(comp); comp.connect(ac.destination);
      noiseBuf = ac.createBuffer(1, Math.floor(ac.sampleRate * 0.35), ac.sampleRate);
      const d = noiseBuf.getChannelData(0); for (let i = 0; i < d.length; i++) d[i] = Math.random() * 2 - 1;
    }
    if (ac.state === "suspended") ac.resume();
  } catch (e) { ac = null; }
  return !!ac;
}
function env(g, t, vol, dur) {
  g.gain.cancelScheduledValues(t);
  g.gain.setValueAtTime(0.0001, t);
  g.gain.linearRampToValueAtTime(vol, t + 0.004);
  g.gain.exponentialRampToValueAtTime(0.0001, t + dur);
}
function tone(f0, f1, dur, type, vol, delay = 0) {
  if (!audioInit()) return;
  const t = ac.currentTime + delay, o = ac.createOscillator(), g = ac.createGain();
  o.type = type; o.frequency.setValueAtTime(f0, t); o.frequency.exponentialRampToValueAtTime(Math.max(20, f1), t + dur);
  env(g, t, vol, dur); o.connect(g); g.connect(master); o.start(t); o.stop(t + dur + 0.03);
}
function noise(dur, vol, cutoff, delay = 0) {
  if (!audioInit()) return;
  const t = ac.currentTime + delay, src = ac.createBufferSource(), f = ac.createBiquadFilter(), g = ac.createGain();
  src.buffer = noiseBuf; f.type = "lowpass"; f.frequency.setValueAtTime(cutoff, t); f.frequency.exponentialRampToValueAtTime(300, t + dur);
  env(g, t, vol, dur); src.connect(f); f.connect(g); g.connect(master); src.start(t, Math.random() * 0.05); src.stop(t + dur + 0.03);
}
const lastPlay = {};
function sfx(name, v = 1) {
  const now = performance.now();
  const gap = { ar: 0, sg: 0, hit: 45, kill: 90, hurt: 120, bot: 70 }[name];
  if (now - (lastPlay[name] || 0) < gap) return;       // limita la sovrapposizione
  lastPlay[name] = now;
  if (name === "ar") { noise(0.09, 0.45, 3200); tone(170, 55, 0.09, "sine", 0.5); }
  else if (name === "sg") { noise(0.2, 0.7, 2400); tone(120, 40, 0.18, "sine", 0.7); noise(0.05, 0.25, 4000, 0.38); tone(320, 180, 0.05, "square", 0.12, 0.4); }
  else if (name === "hit") tone(1250, 900, 0.045, "triangle", 0.16);
  else if (name === "kill") tone(520, 780, 0.11, "triangle", 0.2);
  else if (name === "hurt") tone(130, 70, 0.12, "square", 0.12);
  else if (name === "bot") { noise(0.06, 0.18 * v, 2600); tone(260, 90, 0.06, "sawtooth", 0.07 * v); }
}

/* ---------- effetti e numeri dei danni ---------- */
const ray = new THREE.Raycaster();
const losRay = new THREE.Raycaster();
function addFx(mesh, life) { scene.add(mesh); fx.push({ mesh, life, max: life }); }
function tracer(from, to, color, thick, life) {
  const tr = new THREE.Mesh(new THREE.BoxGeometry(thick, thick, 1), new THREE.MeshBasicMaterial({ color, transparent: true }));
  tr.position.copy(from).lerp(to, 0.5); tr.scale.z = from.distanceTo(to); tr.lookAt(to);
  addFx(tr, life);
}
function showDmg(point, amount, cls) {
  const v = point.clone().project(camera);
  if (v.z > 1) return;
  const el = document.createElement("div");
  el.className = "dmg " + cls; el.textContent = amount;
  el.style.left = ((v.x * 0.5 + 0.5) * stage.clientWidth) + "px";
  el.style.top = ((-v.y * 0.5 + 0.5) * stage.clientHeight) + "px";
  el.style.setProperty("--dx", (Math.random() * 36 - 18) + "px");
  el.addEventListener("animationend", () => el.remove());
  dmgsEl.appendChild(el);
  while (dmgsEl.childElementCount > 24) dmgsEl.firstChild.remove();
}
function damageBot(b, dmg) {          // prima lo scudo, poi la vita
  const s = Math.min(b.sh, dmg); b.sh -= s; b.hp -= dmg - s; updateBotBars(b);
  return { sh: s, hp: dmg - s };
}

/* ---------- sparo del giocatore ---------- */
function fire() {
  const W = WEAPONS[cur];
  sfx(W.sfx);
  rec = Math.min(rec + W.rec * (1 - 0.35 * aimT), 0.14);
  const moving = keys.KeyW || keys.KeyA || keys.KeyS || keys.KeyD;
  // dispersione: meno da accovacciati e molto meno mirando, di piu' in movimento
  const spread = W.spread * (1 - 0.4 * player.crouch) * (1 - 0.65 * aimT) * (moving ? 1.5 : 1);
  ray.setFromCamera({ x: 0, y: 0 }, camera);            // centro dello schermo = dove sta il mirino
  const origin = ray.ray.origin.clone(), base = ray.ray.direction.clone();
  const right = new THREE.Vector3(1, 0, 0).applyQuaternion(camera.quaternion), up = new THREE.Vector3(0, 1, 0).applyQuaternion(camera.quaternion);
  const alive = bots.filter(b => !b.dead).map(b => b.g);
  const targets = [...alive, ...obstacleMeshes, ground];
  guns[cur].updateWorldMatrix(true, true);
  const from = muzzle().getWorldPosition(new THREE.Vector3());
  const per = new Map();                                 // danni aggregati per bot: un solo numero per sparo
  for (let i = 0; i < W.pellets; i++) {
    const a = Math.random() * Math.PI * 2, r = spread * Math.sqrt(Math.random());
    const dir = base.clone().addScaledVector(right, Math.cos(a) * r).addScaledVector(up, Math.sin(a) * r).normalize();
    ray.set(origin, dir); ray.far = 150;
    const hit = ray.intersectObjects(targets, true)[0];
    const end = hit ? hit.point.clone() : origin.clone().addScaledVector(dir, 120);
    tracer(from, end, 0xfff0b0, W.pellets > 1 ? 0.015 : 0.025, 0.07);
    if (!hit) continue;
    let o = hit.object, bot = null;
    while (o && !bot) { bot = o.userData.bot; o = o.parent; }
    if (bot && !bot.dead) {
      const head = hit.object.name === "head";
      let dmg = W.dmg * (head ? W.head : 1);
      if (W.falloff) { const d = Math.max(0, hit.distance - 4.2); dmg *= d < 8 ? 1 : Math.max(0.35, 1 - (d - 8) / 28); }
      dmg = Math.round(dmg);
      const r2 = damageBot(bot, dmg); bot.hitFlash = 0.09;
      const e = per.get(bot) || { total: 0, hpPart: 0, head: false, point: hit.point.clone() };
      e.total += dmg; e.hpPart += r2.hp; e.head = e.head || head; per.set(bot, e);
    } else {
      const sp = new THREE.Mesh(new THREE.BoxGeometry(0.1, 0.1, 0.1), new THREE.MeshBasicMaterial({ color: 0xffd9a0, transparent: true }));
      sp.position.copy(end); addFx(sp, 0.16);
    }
  }
  const fl = new THREE.Mesh(new THREE.SphereGeometry(cur === 1 ? 0.18 : 0.11, 8, 6), new THREE.MeshBasicMaterial({ color: 0xffe9a0, transparent: true }));
  fl.position.copy(from); addFx(fl, 0.05);

  for (const [bot, e] of per) {
    showDmg(e.point, e.total, e.head ? "head" : e.hpPart > 0 ? "hp" : "sh");
    G.hitUntil = performance.now() + 140; G.hitHead = e.head;
    if (bot.hp <= 0 && !bot.dead) {
      bot.dead = true; bot.t = 0; bot.bars.visible = false; setKills(kills + 1); G.killUntil = performance.now() + 260;
      player.sh = Math.min(100, player.sh + 20); updateVitals();      // ricompensa: +20 scudo per eliminazione
      sfx("kill");
    } else sfx("hit");
  }
}

/* ---------- sparo dei bot ---------- */
function hasLOS(from, to) {
  const d = to.clone().sub(from), len = d.length();
  losRay.set(from, d.normalize()); losRay.far = len;
  return losRay.intersectObjects(obstacleMeshes, false).length === 0;
}
function botShoot(b, dist, target) {
  const fwd = new THREE.Vector3(-Math.sin(b.g.rotation.y), 0, -Math.cos(b.g.rotation.y));
  const from = new THREE.Vector3(b.g.position.x, 1.35, b.g.position.z).addScaledVector(fwd, 0.55);
  const chance = Math.max(0.1, Math.min(0.5, 0.5 - dist * 0.012)) * (1 - 0.35 * player.crouch);
  if (Math.random() < chance) { tracer(from, target, 0xff6a5a, 0.03, 0.08); hurtPlayer(5); }
  else {
    const miss = target.clone().add(new THREE.Vector3((Math.random() - 0.5) * 2.4, (Math.random() - 0.3) * 1.6, (Math.random() - 0.5) * 2.4));
    tracer(from, miss, 0xff6a5a, 0.02, 0.08);
  }
  sfx("bot", 1 / (1 + dist / 10));
}

/* ---------- camera / loop ---------- */
const zoomFactor = () => 1 - aimT * WEAPONS[cur].zoom;       // mirando il FOV si restringe
function applyFov() {
  // FOV orizzontale (convenzione dei giochi), convertito nel verticale richiesto da three.js
  const h = G.fov * zoomFactor();
  const v = THREE.MathUtils.radToDeg(2 * Math.atan(Math.tan(THREE.MathUtils.degToRad(h) / 2) / camera.aspect));
  camera.fov = v; camera.updateProjectionMatrix(); lastFov = h;
}
let lastFov = 0;
function resize() {
  const w = Math.max(2, stage.clientWidth), h = Math.max(2, stage.clientHeight);
  renderer.setSize(w, h, false); camera.aspect = w / h; applyFov();
}
new ResizeObserver(resize).observe(stage); resize();

setWeapon(0); updateVitals(); idleScene();
if (coarse) { startMsg.textContent = (T().desktop || "Serve un computer con mouse e tastiera."); stage.dataset.nolock = "1"; startEl.hidden = false; }
else showStart("play");

const clock = new THREE.Clock();
const tmpV = new THREE.Vector3();
function animate() {
  requestAnimationFrame(animate);
  const dt = Math.min(clock.getDelta(), 0.05);
  const live = state.locked && !state.over;
  const pos = player.g.position;

  aimT += ((aiming && live ? 1 : 0) - aimT) * Math.min(1, dt * 12);
  if (Math.abs(G.fov * zoomFactor() - lastFov) > 0.01) applyFov();

  if (live) {
    // accovacciarsi (C o Ctrl): piu' lento, bersaglio piu' piccolo, meno dispersione
    const wantCrouch = keys.KeyC || keys.ControlLeft ? 1 : 0;
    player.crouch += (wantCrouch - player.crouch) * Math.min(1, dt * 12);
    const c = player.crouch;
    // movimento relativo alla visuale
    let f = (keys.KeyW ? 1 : 0) - (keys.KeyS ? 1 : 0), s = (keys.KeyD ? 1 : 0) - (keys.KeyA ? 1 : 0);
    const l = Math.hypot(f, s) || 1; f /= l; s /= l;
    const sprint = (keys.ShiftLeft || keys.ShiftRight) && c < 0.2 && aimT < 0.3;
    const sp = (sprint ? 7.4 : 5) * (1 - 0.5 * c) * (1 - 0.35 * aimT);
    pos.x += (-Math.sin(yaw) * f + Math.cos(yaw) * s) * sp * dt;
    pos.z += (-Math.cos(yaw) * f - Math.sin(yaw) * s) * sp * dt;
    pos.x = Math.max(-ARENA, Math.min(ARENA, pos.x)); pos.z = Math.max(-ARENA, Math.min(ARENA, pos.z));
    pushOut(pos, 0.4);
    if (keys.Space && pos.y <= 0.001 && c < 0.5) player.vy = 6.2;
    player.vy -= 18 * dt; pos.y = Math.max(0, pos.y + player.vy * dt); if (pos.y === 0) player.vy = 0;
    player.walk += (f || s ? sp : 0) * dt * 1.7;
    // sparo
    fireT -= dt;
    if (firing && fireT <= 0) { fire(); fireT = WEAPONS[cur].rate; }
    // spawn
    const maxAlive = Math.min(8, 3 + Math.floor(kills / 5));
    spawnT -= dt;
    if (spawnT <= 0 && bots.filter(b => !b.dead).length < maxAlive) { const p = randomSpawn(); spawnBot(p.x, p.z); spawnT = 1.4; }
    rec = Math.max(0, rec - dt * 0.14);
  } else {
    player.crouch += (0 - player.crouch) * Math.min(1, dt * 8);
  }
  player.g.rotation.y = yaw;
  player.g.scale.y = 1 - 0.32 * player.crouch;
  const moving = live && (keys.KeyW || keys.KeyA || keys.KeyS || keys.KeyD);
  const swing = Math.sin(player.walk) * 0.7 * (moving ? 1 : 0);
  const u = player.g.userData;
  u.legL.rotation.x = swing; u.legR.rotation.x = -swing; u.armL.rotation.x = -swing; u.armR.rotation.x = -1.45 + (pitch + rec) * 0.9;
  guns.forEach(g => { g.rotation.x = (pitch + rec) * 0.9; g.position.y = 1.3 + (pitch + rec) * 0.08; });

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
      // posizionamento: si avvicina, mantiene la distanza preferita e si sposta di lato
      let mx = 0, mz = 0, speedMul = 1;
      if (d > b.pref + 2) { mx = dx / d; mz = dz / d; }
      else if (d < b.pref - 3) { mx = -dx / d; mz = -dz / d; speedMul = 0.6; }
      else { b.strafeT -= dt; if (b.strafeT <= 0) { b.strafe *= -1; b.strafeT = 1 + Math.random() * 2; } mx = -dz / d * b.strafe; mz = dx / d * b.strafe; speedMul = 0.55; }
      for (const o of bots) { if (o === b || o.dead) continue; const ox = b.g.position.x - o.g.position.x, oz = b.g.position.z - o.g.position.z, od = Math.hypot(ox, oz); if (od < 1.3 && od > 0.01) { mx += ox / od * 0.8; mz += oz / od * 0.8; } }
      const ml = Math.hypot(mx, mz);
      if (ml > 0.01) { const sp = (b.speed + Math.min(0.9, kills / 40)) * speedMul; b.g.position.x += mx / ml * sp * dt; b.g.position.z += mz / ml * sp * dt; pushOut(b.g.position, 0.4); b.walk += dt * 6; }
      // linea di tiro libera? (controllata ogni ~0.25 s)
      b.losT -= dt;
      const target = tmpV.set(pos.x, pos.y + 1.15 - 0.4 * player.crouch, pos.z).clone();
      if (b.losT <= 0) { b.los = hasLOS(new THREE.Vector3(b.g.position.x, 1.5, b.g.position.z), target); b.losT = 0.25; }
      // raffiche da 3 colpi
      b.cool -= dt;
      if (b.burst > 0) {
        b.shotT -= dt;
        if (b.shotT <= 0) { botShoot(b, d, target); b.burst--; b.shotT = 0.13; if (!b.burst) b.cool = 1.0 + Math.random() * 1.4; }
      } else if (b.cool <= 0 && d < 40 && b.los) { b.burst = 3; b.shotT = 0; }
      bu.legL.rotation.x = Math.sin(b.walk) * 0.7; bu.legR.rotation.x = -Math.sin(b.walk) * 0.7;
      bu.armL.rotation.x = -Math.sin(b.walk) * 0.5; bu.armR.rotation.x = -1.4;
    }
    if (b.hitFlash > 0) { b.hitFlash -= dt; b.g.traverse(o => { if (o.isMesh && o.material.emissive) o.material.emissive.setScalar(b.hitFlash > 0 ? 0.7 : 0); }); }
  }
  bots = bots.filter(b => { if (b.dead && b.t > 1.7) { removeBot(b); return false; } return true; });

  if (live && player.hp <= 0) {
    state.over = true; state.needsReset = true; G.playing = false; document.exitPointerLock?.(); showStart("over");
  }
  hurtEl.style.opacity = Math.max(0, parseFloat(hurtEl.style.opacity || 0) - dt * 1.4);

  // effetti
  for (let i = fx.length - 1; i >= 0; i--) {
    const e = fx[i]; e.life -= dt; const k = Math.max(0, e.life / e.max);
    e.mesh.material.opacity = k; if (e.life <= 0) { scene.remove(e.mesh); e.mesh.geometry.dispose(); e.mesh.material.dispose(); fx.splice(i, 1); }
  }

  // camera in terza persona (spalla destra); mirando si avvicina; collisione contro i ripari
  const p = pitch + rec;
  const head = new THREE.Vector3(pos.x, pos.y + 1.55 - 0.55 * player.crouch, pos.z);
  const aim = new THREE.Vector3(-Math.sin(yaw) * Math.cos(p), Math.sin(p), -Math.cos(yaw) * Math.cos(p));
  const right = new THREE.Vector3(Math.cos(yaw), 0, -Math.sin(yaw));
  const want = head.clone().addScaledVector(aim, -(4.2 - 2.0 * aimT)).addScaledVector(right, 0.85 - 0.3 * aimT); want.y += 0.25 - 0.1 * aimT;
  const dir = want.clone().sub(head), dist = dir.length(); dir.normalize();
  ray.set(head, dir); ray.far = dist;
  const ch = ray.intersectObjects(obstacleMeshes, false)[0];
  if (ch) want.copy(head).addScaledVector(dir, Math.max(0.4, ch.distance - 0.3));
  if (want.y < 0.3) want.y = 0.3;
  camera.position.copy(want); camera.rotation.set(p, yaw, 0);

  // barre dei bot: sopra la testa, sempre rivolte alla camera
  for (const b of bots) { if (b.dead) continue; b.bars.position.set(b.g.position.x, 2.2, b.g.position.z); b.bars.quaternion.copy(camera.quaternion); }

  renderer.render(scene, camera);
}
animate();
