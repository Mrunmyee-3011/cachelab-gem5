# GEM5 validation for CacheLab

Goal: produce **real GEM5 `stats.txt` files** for the same atomic-counter workload that CacheLab simulates, then import them in
CacheLab → **GEM5 / Multi2Sim** page and compare trends.

| File | Purpose |
|---|---|
| `booking.c` | Workload: N threads, each does `iters` atomic increments. `mode 0` = one shared counter (true sharing), `mode 1` = one padded counter per thread (no sharing) |
| `se_mesi.py` | GEM5 config: X86, N cores, Ruby **MESI_Two_Level**, syscall-emulation mode |
| `run_all.sh` | Runs 2/4/8 cores × shared/padded and collects the stats files |
| `github-actions-gem5.yml` | Optional: builds GEM5 on GitHub's servers so nothing is built on your PC |

GEM5 runs on Linux only. Pick **one** way to get a `gem5.opt` binary.

## Option A (least effort on your PC): GitHub Actions
1. Create a free GitHub repository and upload this `gem5-validation` folder to it.
2. Move `github-actions-gem5.yml` to `.github/workflows/gem5.yml` in the repository (keep the folder `gem5-validation/` as is).
3. Open the **Actions** tab → *gem5 validation* → **Run workflow**. It builds GEM5 (about 40–90 min), runs all 6 simulations and uploads `stats` as an artifact.
4. Download the artifact zip and unzip it. You get six files such as `shared_4c.stats.txt`. Go to **Step 3** below.
   (Nothing is installed on your C: drive.)

## Option B: WSL2 on your PC (keep everything on D:)
1. In PowerShell (admin): `wsl --install -d Ubuntu`, reboot, create the Linux user.
2. Your C: drive is small, so move the distro to D:
   ```powershell
   wsl --export Ubuntu D:\wsl\ubuntu.tar
   wsl --unregister Ubuntu
   wsl --import Ubuntu D:\wsl\Ubuntu D:\wsl\ubuntu.tar
   ```
   (Skip this if the distro is already on D: or C: has plenty of space.)
3. In Ubuntu:
   ```bash
   sudo apt update
   sudo apt install -y build-essential git m4 scons python3-dev python3-pip zlib1g-dev \
        libprotobuf-dev protobuf-compiler libprotoc-dev libgoogle-perftools-dev libboost-all-dev pkg-config
   git clone --depth 1 --branch v25.1.0.1 https://github.com/gem5/gem5 ~/gem5
   cd ~/gem5
   scons build/X86/gem5.opt -j4        # X86 build uses Ruby protocol MESI_Two_Level. Takes 40-90+ min and ~8 GB RAM
   ```
4. Run the workload (copy `gem5-validation` into Ubuntu's home first, e.g. from `/mnt/d/...`):
   ```bash
   cd ~/gem5-validation
   GEM5=~/gem5/build/X86/gem5.opt bash run_all.sh 1000
   ```
   Results appear in `results/stats/`. Copy that folder back to Windows (e.g. `cp -r results/stats /mnt/d/...`).

## Step 3: import into CacheLab
1. In `cachelab/`: `npm run dev`, open the **GEM5 / Multi2Sim** page.
2. For each file set the four fields first (**Protocol** = MESI_Two_Level, **Cores**, **Workload** = shared or padded), then choose the file.
   `shared_4c.stats.txt` → cores 4, workload "shared counter".
3. Read section 3 (CacheLab twin): compare the **shared ÷ padded** ratio and the way invalidations grow with cores in both tools.
4. Use section 4 (raw browser) to search for `Inv`, `Fwd_GET`, `m_demand`, `msg_count`.

## What to say about the results (honest scope)
- GEM5 measures a full x86 program (loop, stack, thread start-up, instruction fetch); CacheLab models only the counter accesses. Absolute numbers differ — compare **trends**.
- The GEM5 build here has **MESI** only. MSI/MOESI exist in CacheLab; GEM5's MOESI protocols are different builds (e.g. `build_opts/NULL_MOESI_CMP_directory`) and are outside this minimal setup.
- CacheLab never edits or invents imported values; anything missing from a file is displayed as "not found".

## If something fails
- Build fails with out-of-memory: use `-j2`.
- `requires(...)` error: the binary was not built as MESI_Two_Level (rebuild `build/X86/gem5.opt` from the v25.1 source).
- Run hangs or errors about threads: the number of cores must equal the thread count (GEM5 SE mode has no time-slicing). `run_all.sh` already does this.
- Attach the exact error text and I can fix the config.

## Multi2Sim (optional)
Multi2Sim 5.0 is old and hard to build on current Linux. Treat it as optional: if you get it running (an old Ubuntu in a container is the usual route), enable its memory report (`--mem-report <file>`), run the same booking program, and import the report on the same page (Tool = Multi2Sim, set the *module prefix* to your private-L1 module names, default `mod-l1`). If you cannot build it, state this in the report and use GEM5 as the real-tool evidence.
