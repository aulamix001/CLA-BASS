#!/usr/bin/env bash
set -euo pipefail
# User-owned tools and extracted, signature-verified Debian development packages.
task_root=/workspace
project_dir="$task_root/CLA-BASS"
tools_dir="$task_root/tools"
sysroot_dir="$task_root/sysroot"
apt_dir="$task_root/apt"
mkdir -p "$tools_dir" "$sysroot_dir" "$apt_dir/lists/partial" "$apt_dir/cache/archives/partial"
python3 -m pip install --upgrade --target "$tools_dir" cmake==4.4.5 ninja==1.13.2
cat > "$apt_dir/sources.list" <<'EOF'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
EOF
apt_options=(-o "Dir::Etc::sourcelist=$apt_dir/sources.list" -o Dir::Etc::sourceparts=-
  -o Dir::Etc::parts=- -o "Dir::State::lists=$apt_dir/lists" -o "Dir::Cache=$apt_dir/cache")
/usr/bin/apt-get "${apt_options[@]}" update
(
  cd "$tools_dir"
  /usr/bin/apt-get "${apt_options[@]}" download libx11-dev libxext-dev libxrandr-dev \
    libxinerama-dev libxcursor-dev libxrender-dev libxfixes-dev libfreetype-dev \
    libfontconfig-dev libasound2-dev x11proto-dev libpng-dev zlib1g-dev \
    libxau-dev libxdmcp-dev libxcb1-dev libbz2-dev
)
python3 - <<'PY'
from pathlib import Path
import subprocess
for p in Path('/workspace/tools').glob('*.deb'):
    subprocess.run(['dpkg-deb','-x',str(p),'/workspace/sysroot'],check=True)
for p in Path('/workspace/sysroot/usr/lib/x86_64-linux-gnu').glob('*.so'):
    if p.is_symlink() and not p.exists():
        runtime=Path('/usr/lib/x86_64-linux-gnu')/p.readlink().name
        if runtime.exists():
            p.unlink()
            p.symlink_to(runtime)
PY
if [ ! -d "$task_root/deps/JUCE" ]; then
  git clone --depth 1 --branch 8.0.6 https://github.com/juce-framework/JUCE.git "$task_root/deps/JUCE"
fi
test "$(git -C "$task_root/deps/JUCE" rev-parse HEAD)" = 51a8a6d7aeae7326956d747737ccf1575e61e209
export CXXFLAGS="-I$sysroot_dir/usr/include -I$sysroot_dir/usr/include/freetype2"
export LDFLAGS="-L$sysroot_dir/usr/lib/x86_64-linux-gnu"
export PKG_CONFIG_PATH="$sysroot_dir/usr/lib/x86_64-linux-gnu/pkgconfig:$sysroot_dir/usr/share/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR="$sysroot_dir"
export CMAKE_BUILD_PARALLEL_LEVEL=2
cmake_bin="$tools_dir/cmake/data/bin/cmake"
"$cmake_bin" -S "$project_dir" -B "$project_dir/build-linux" \
  -DJUCE_DIR="$task_root/deps/JUCE" -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_CXX_FLAGS_RELEASE=-O1 -DNDEBUG' '-DCMAKE_C_FLAGS_RELEASE=-O1 -DNDEBUG'
"$cmake_bin" --build "$project_dir/build-linux" --parallel 2
"$tools_dir/cmake/data/bin/ctest" --test-dir "$project_dir/build-linux" --output-on-failure
