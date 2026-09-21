Mandatory rules for AI coding agents contributing to this repo. Direct user instructions win — flag the conflict when they do.

## Repository layout

```
├── .clang-format, CPPLINT.cfg     # C/C++ style rules for the ESP32 firmware (IDF_esp32p4_webrtc/)
├── .gitignore                     # Covers both this repo and the ESP-IDF firmware dir (build/, sdkconfig, managed_components/, ...)
├── run.sh, stop.sh                # Thin wrappers forwarding to cloud_center/run.sh / stop.sh — it's the only stack
├── doc/
│   ├── design.md                  # Full architecture: device registration protocol, registry model, security model — read before changing cloud_center/
│   ├── MQTT_WEBRTC_CONTROL_FLOW.md# MQTT command/status protocol driving WebRTC lifecycle on ESP32 + robot/servo control
│   └── ALIYUN_ECS_DEPLOYMENT.md   # Aliyun ECS-specific firewall/deployment walkthrough
├── IDF_esp32p4_webrtc/            # ESP32-P4 firmware — not yet updated to doc/design.md §3's dynamic registration protocol
└── cloud_center/                  # The one deployment: dashboard + registry/provisioning backend + Janus + Mosquitto
    ├── README.md                  # What's implemented vs. still open (currently: the firmware side)
    ├── docker-compose.yml         # backend, caddy, janus, mosquitto — backend/caddy/janus are network_mode: host
    ├── caddy/Caddyfile            # Two site blocks: public :8090 (dashboard, forward_auth-gated registry/API, Janus/Mosquitto viewer proxies), LAN-only :8092 (/device/register only, remote_ip-gated)
    ├── backend/                   # Session auth + device registry + Janus/Mosquitto provisioning (formerly auth-gateway/ — renamed, its scope grew past just auth)
    │   ├── server.py               # All routes; depends on paho-mqtt (the one non-stdlib dependency in this repo, deliberate — see cloud_center/README.md)
    │   └── hash_token.py           # CLI: generate a CLOUD_CENTER_TOKEN_HASH value from a plaintext token
    ├── scripts/
    │   ├── bootstrap_secrets.sh    # Generates every secret in .env + janus/*.jcfg + seeds Mosquitto's dynamic-security store; called by run.sh
    │   └── check_firewall.sh       # Checks ufw status against doc/README.md's Ports table; prints ufw commands to fix mismatches
    ├── run.sh, stop.sh            # Seed web/devices.json + bootstrap_secrets.sh, then `docker compose up -d --build` / `down`; --clean flag to wipe state
    ├── .env.example               # Every required secret's placeholder, with a comment on how each is generated/used
    ├── .gitignore                 # .env, web/devices.json, mosquitto/data/ (real config/secrets) gitignored
    ├── janus/*.jcfg                # Janus SFU/transport config — no rooms pre-defined; every room is created/destroyed at runtime via the plugin's HTTP API
    ├── mosquitto/mosquitto.conf    # Dynamic-security plugin enabled — no static password file; users/roles/ACLs created at runtime by backend/server.py
    ├── create_domain.md           # DNS/domain setup for Caddy's automatic TLS
    └── web/
        ├── devices.example.json   # Registry schema reference — real registry (web/devices.json) is populated by device self-registration, not hand-edited
        ├── login.html, login.js   # Public sign-in page — the only route not gated by forward_auth (on the public listener)
        ├── auth.js                # wireSignOut() — POSTs /auth/logout, used by every other page's topbar
        ├── index.html, overview.js       # Overview: device grid, polls GET /registry/devices (no browser-side MQTT connection)
        ├── live.html, live.js, device-session.js  # Live View: video + capability-conditional robot/servo controls; MQTT via the shared viewer credential
        ├── devices.html, devices-page.js  # Rename/revoke devices (no manual "add" — devices only arrive via self-registration)
        └── vendor/                # Vendored adapter.js, janus.js — third-party, do not hand-edit
```

## 1. Read the docs before starting a task

| Doc | Covers |
|---|---|
| `doc/design.md` | Full architecture: device registration/provisioning protocol, registry data model, security model, failure modes — read before touching anything in `cloud_center/` |
| `cloud_center/README.md` | What's implemented vs. still open (currently: ESP32 firmware isn't updated to the new protocol yet); how to run it locally; port table |
| `doc/MQTT_WEBRTC_CONTROL_FLOW.md` | MQTT command/status protocol driving WebRTC lifecycle on the ESP32 |
| `doc/ALIYUN_ECS_DEPLOYMENT.md` | Aliyun ECS firewall/security-group rules and deployment steps |
| `cloud_center/create_domain.md` | Buying/configuring a domain so Caddy can issue TLS certs |
| `cloud_center/scripts/bootstrap_secrets.sh --help` | Flags for generating and propagating every secret this stack needs |
| `cloud_center/run.sh`, `stop.sh` | Thin wrapper: seed `web/devices.json` if missing, run the bootstrap script, then `docker compose up -d --build` / `down` |

## 2. Update docs before opening a PR

Ship the doc fix with the code — not as a follow-up.

* Changes to `docker-compose.yml`, `janus/*.jcfg`, `mosquitto.conf`, `caddy/Caddyfile`, or the security controls/deployment steps they implement → update `cloud_center/README.md` (and `doc/design.md` if the change alters the architecture itself, not just an implementation detail).
* Aliyun-specific firewall or ECS steps → update `ALIYUN_ECS_DEPLOYMENT.md`.
* MQTT topic names or the WebRTC start/stop command protocol → update `MQTT_WEBRTC_CONTROL_FLOW.md`.
* New/changed `.env` keys → update `.env.example` with a comment, and `README.md` wherever that key is referenced.
* New `devices.json` fields → update `cloud_center/web/devices.example.json` and the relevant page in `cloud_center/web/`.
* Any change to the device registration protocol (request/response shape, what gets provisioned when) → update `doc/design.md` §3 — it's the spec the (not-yet-written) firmware side will implement against.

## 3. Validation before opening a PR

There is no automated test suite; this is an infrastructure/config repo. Validate manually, matching what changed:

* `docker-compose.yml`, Caddy, Janus, or Mosquitto config → `cd cloud_center && docker compose config` to catch syntax errors, then `docker compose up -d` and check `docker compose logs` for the affected service.
* Shell scripts → `bash -n <script>` at minimum; prefer running it against a scratch `.env`/`jcfg` copy before trusting it against real secrets.
* `backend/server.py` changes touching Mosquitto's dynamic-security integration → confirm the exact command/response shape against the actual installed Mosquitto version (`doc/design.md` and the code both flag this as unverified-by-construction, since there's no automated test for it) — don't assume the JSON schema in comments is exactly right without checking a real broker.
* `cloud_center/web/` → `docker compose up -d --build` and check the Overview/Live View/Devices pages in a browser, including registering a test device via `curl` (see `cloud_center/README.md`) and confirming it appears, goes "online" after a fake MQTT publish, and can be revoked (confirm the revoke actually deletes the Mosquitto client and Janus room, not just the registry row).
* `backend/server.py` or `caddy/Caddyfile`'s `forward_auth`/public-path matcher → `python3 -m py_compile server.py`, then confirm in a browser: signed-out access to any non-login page redirects to `/login.html?next=...`, a correct token signs in and lands back on that page, an incorrect token shows an inline error (not a crash); confirm `POST /device/register` works against `:8092` and is rejected against `:8090`.

## 4. Secrets discipline

* `janus/*.jcfg` are committed with placeholder `admin_secret`/`admin_key` values. `bootstrap_secrets.sh` overwrites these files in place with real generated secrets — after running it locally, check `git diff` before committing and never let real secrets replace the placeholders in git history.
* `.env` (as opposed to `.env.example`) holds every real secret and is gitignored — never add or force-add it.
* `cloud_center/web/vendor/` (adapter.js, janus.js) is vendored third-party code — update it by replacing the file wholesale from upstream, not by hand-editing.
* `cloud_center/web/devices.json` and `cloud_center/mosquitto/data/` (real registered devices' Janus room pins, and Mosquitto's dynamic-security credential store) and `cloud_center/.env` are all gitignored — only `devices.example.json` and `.env.example` (placeholders) are committed.
* `cloud_center/.env` needs `CLOUD_CENTER_TOKEN_HASH`, `CLOUD_CENTER_SESSION_SECRET`, `JANUS_VIDEOROOM_ADMIN_KEY`, `CLOUD_CENTER_MOSQUITTO_ADMIN_USERNAME`/`PASSWORD`, and `CLOUD_CENTER_MQTT_VIEWER_USERNAME`/`PASSWORD` — the backend fails to start without all of them (they're read via `os.environ[...]`, not `.get()` with a default) — never make any of them optional.
* Every path except `/login.html`, `/login.js`, `/styles.css` (on the public `:8090` listener) must stay behind Caddy's `forward_auth` to `backend:8091/auth/check` — don't widen that public-path matcher without an equally strong reason. `/device/register` lives on the separate LAN-only `:8092` listener instead (§6 of `doc/design.md`) — never move it onto `:8090`, and never remove `:8092`'s `remote_ip private_ranges` matcher.

## 5. Destructive-action discipline

Follow the harness's default git-safety protocol (no force-push, `git reset --hard`, branch deletion, or `--no-verify` without explicit user confirmation this session). Repo-specific additions:

* `docker compose down` / `restart` on `janus` or `mosquitto` drops every live ESP32 connection and in-progress viewer session across the *entire* fleet, not just one site — this is a direct consequence of collapsing to a single deployment (`doc/design.md` §8). Confirm with the user before running against a stack that might be serving real devices.
* Firewall/security-group changes described in the deployment docs happen outside this repo (cloud console, `ufw`) — treat instructions to open/close ports as requiring the same confirmation as any other change to shared infrastructure. Opening `:8092` (device registration) beyond a trusted local network is a security-relevant change, not routine port hygiene — flag it explicitly rather than just doing it.

## 6. Scope discipline

* Broad reshuffles ship separately. A drive-by within files you're already touching is fine; a sweep across unrelated services (e.g. touching Mosquitto config while fixing Caddy) is its own PR.
* No backwards-compatibility shims or feature flags for hypothetical deployments. There is one deployment target (`cloud_center/`'s Docker Compose stack); change it directly.
* `cloud_center/` never proxies media or MQTT commands through the backend — the browser and devices always talk to Janus/Mosquitto directly (`doc/design.md` §2). Don't add a media/command relay path to `backend/server.py` "for convenience."
* No multi-tenant/multi-operator auth model, no multi-SFU or multi-broker abstraction, until actually asked for — see `doc/design.md` §1's non-goals.

## 7. Prefer the simplest design that solves the concrete problem

Don't add abstraction layers, configurable knobs, or extension points "in case we need them later." Drop any proposal bullet that starts with "this also lets us…" or "leaves room for…". The one accepted exception already in this repo: `backend/` depends on `paho-mqtt` rather than hand-rolling MQTT wire framing in stdlib — that was a deliberate call (see `cloud_center/README.md`), not a precedent for adding dependencies casually elsewhere.

## 8. Name things fully; avoid abbreviations

Prefer the unabbreviated word for new files, directories, env vars, and config keys. Keep abbreviations already established in this repo (`.env`, `MQTT`, `PIN`, `FQDN`, `SFU`) rather than re-deriving new ones.
