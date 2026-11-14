# souldew-3ds build image: devkitARM + libctru + CTRPluginFramework (libctrpf) + 3gxtool.
# Pinned: the devkitARM image by digest, 3gxtool by commit. libctrpf comes from ThePixellizerOSS' package
# database (the version the build saw is printed by build.sh and written to build/TOOLCHAIN.txt).
FROM devkitpro/devkitarm@sha256:116afba8df8453961de2936ffab20dd441edf4d682856c1ec8b0e53d7ed0bbf5

ARG GXTOOL_COMMIT=57e3160ab72d8533a2f0601efdc8a149b4825ea5
ARG MAKEROM_TAG=makerom-v0.18.4

RUN printf '\n[thepixellizeross-lib]\nServer = https://thepixellizeross.gitlab.io/packages/any\nSigLevel = Optional\n' \
      >> /opt/devkitpro/pacman/etc/pacman.conf \
 && dkp-pacman -Sy --noconfirm \
 && dkp-pacman -S --noconfirm --needed libctrpf \
 && dkp-pacman -Scc --noconfirm

# 3gxtool turns the plugin's ELF into a .3gx (the prebuilt one is x86_64 only – built here for any host arch)
RUN git clone --recursive https://gitlab.com/thepixellizeross/3gxtool.git /tmp/3gxtool \
 && cd /tmp/3gxtool && git checkout -q ${GXTOOL_COMMIT} && git submodule update --init --recursive \
 && mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release >/dev/null && make -j4 \
 && install -m755 $(find . -maxdepth 2 -type f -name 3gxtool | head -1) /opt/devkitpro/tools/bin/3gxtool \
 && rm -rf /tmp/3gxtool

# makerom builds the mock game (a test title with its own title ID, so Azahar's plugin loader runs the plugin in it)
# and the launcher's .cia; ctrtool (same repository and tag) is only used to check the .cia's structure
RUN git clone --depth 1 --branch ${MAKEROM_TAG} https://github.com/3DSGuy/Project_CTR.git /tmp/ctr \
 && cd /tmp/ctr/makerom && make deps >/dev/null && make -j4 >/dev/null \
 && install -m755 $(find . -type f -name makerom -perm -u+x | head -1) /opt/devkitpro/tools/bin/makerom \
 && cd /tmp/ctr/ctrtool && make deps >/dev/null && make -j4 >/dev/null \
 && install -m755 $(find . -type f -name ctrtool -perm -u+x | head -1) /opt/devkitpro/tools/bin/ctrtool \
 && rm -rf /tmp/ctr

# bannertool makes the .cia's HOME menu banner (image + sound). carstene1ns' maintained fork of Steveice10's tool
# (the original repository is gone), pinned by commit (tag 1.2.3); its CMake minimum is lowered to the image's CMake.
ARG BANNERTOOL_COMMIT=da573b13418acd0ac1803b43b30e8a155e5f83e2
RUN git clone https://github.com/carstene1ns/3ds-bannertool.git /tmp/bannertool \
 && cd /tmp/bannertool && git checkout -q ${BANNERTOOL_COMMIT} \
 && sed -i 's/cmake_minimum_required(VERSION 3.28)/cmake_minimum_required(VERSION 3.18)/' CMakeLists.txt \
 && cmake -B build -DCMAKE_BUILD_TYPE=Release >/dev/null && cmake --build build -j4 >/dev/null \
 && install -m755 build/bannertool /opt/devkitpro/tools/bin/bannertool \
 && rm -rf /tmp/bannertool

ENV DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM
WORKDIR /src
