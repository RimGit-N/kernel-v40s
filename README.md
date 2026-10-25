# ZTE Blade V40s (ZTE 9047) Kernel Source - NevaCPS Tempest

Kernel source tree untuk ZTE Blade V40s (ZTE 9047 / Unisoc T618 - UMS512 / SharkL5Pro).

- **Kernel Version**: `5.4.161`
- **Branding Target**: `5.4.161-NevaCPS-12-9-gb17cbc51204f-tempest`
- **Architecture**: `ARM64` (`ARCH=arm64`)
- **Default Defconfig**: `sprd_sharkl5Pro_defconfig` (alternatif: `gki_defconfig`)
- **Toolchain**: Android Clang `r416183b` (Clang 12.0.5/12.0.7) + AArch64 GNU Cross Toolchain

---

## Struktur Direktori

```text
├── .github/workflows/
│   └── kernel-build.yml       # CI/CD otomatis via GitHub Actions
├── bsp/
│   └── kernel/
│       └── kernel5.4/         # Source code kernel Linux 5.4.161
├── external/                  # Userland dependencies (iptables, iproute2)
├── .gitignore                 # Filter file output / sementara
├── build.sh                   # Script helper build lokal / Linux
└── README.md                  # Dokumentasi ini
```

---

## Panduan Lengkap untuk Pemula

### 1. File & Folder yang Wajib Masuk GitHub
- `bsp/` (seluruh source code kernel).
- `external/` (modul external bawaan BSP).
- `.github/` (workflow CI/CD).
- `.gitignore`, `build.sh`, `README.md`.

### 2. File & Folder yang HARUS Diabaikan (Jangan Diunggah)
- `out/` atau folder output build.
- `toolchain/` atau binary compiler besar (`clang-*.tar.gz`).
- `build.log` atau file log sementara.
- File backup editor/patch: `*.orig`, `*.rej`, `*.bak`.

### 3. Cara Memeriksa Ukuran Repository Sebelum Upload
Jalankan di terminal Git Bash:
```bash
du -sh .
```
Pastikan folder `out/` atau archive compiler tidak ikut terhitung.

### 4. Cara Memeriksa File Besar (> 100 MB)
GitHub menolak file individual di atas 100 MB tanpa Git LFS:
```bash
find . -type f -size +100M -not -path "*/.git/*"
```
Jika tidak ada output, repository aman untuk di-push ke GitHub.

### 5. Cara Membuat Repository di GitHub
1. Buka [github.com/new](https://github.com/new).
2. Isi Repository name: misalnya `kernel_zte_v40s`.
3. Pilih **Private** atau **Public**.
4. **JANGAN** centang "Add a README file" atau .gitignore (karena sudah dibuat secara lokal).
5. Klik **Create repository**.

### 6. Cara Inisialisasi Git & Push Branch Pertama dengan Aman
Jalankan perintah berikut di root folder `kernel-v40s`:
```bash
# Inisialisasi repository git
git init -b main

# Tambahkan file yang sudah difilter oleh .gitignore
git add .

# Buat commit baseline
git commit -m "feat(kernel): initial commit with NevaCPS tempest rebranding"

# Hubungkan ke repository GitHub Anda (ganti URL sesuai repo Anda)
git remote add origin https://github.com/USERNAME/REPO_NAME.git

# Push ke GitHub
git push -u origin main
```

### 7. Cara Menjalankan GitHub Actions
1. Masuk ke halaman repository Anda di GitHub.
2. Klik tab **Actions** di bagian atas.
3. Di sisi kiri, pilih workflow **Build Android Kernel ARM64**.
4. Klik tombol **Run workflow** (di sebelah kanan).
5. Pilih defconfig (`sprd_sharkl5Pro_defconfig` direkomendasikan) lalu klik tombol hijau **Run workflow**.

### 8. Cara Mengunduh Artifact Build
1. Tunggu proses build di tab **Actions** selesai (status hijau bertanda centang).
2. Klik pada run workflow yang selesai tersebut.
3. Scroll ke bagian paling bawah ke seksi **Artifacts**.
4. Unduh file `kernel-5.4.161-NevaCPS-12-9-gb17cbc51204f-tempest.zip` yang berisi:
   - `Image`
   - `Image.gz`
   - `kernel.config`
   - `kernel.release`
   - `build.log`

### 9. Cara Membaca Log Jika Build Gagal
1. Klik pada step yang berwarna merah di run workflow.
2. Buka step **Build Kernel Image**.
3. Cari baris bertuliskan `error:` atau `fatal error:`.
4. Unduh `build.log` dari artifacts untuk membaca log kompilasi secara menyeluruh.

---

## Lisensi & Hak Cipta
Source code kernel dilisensikan di bawah GNU General Public License v2 (GPL-2.0). Lihat file `bsp/kernel/kernel5.4/COPYING` dan direktori `LICENSES/` untuk detail lengkap.
