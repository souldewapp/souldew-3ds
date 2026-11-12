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
RUN git clone --depth 1 --branch ${MAKEROM_TAG} https://github.com/3DSGuy/Project_CTR.git /tmp/ctr \
 && cd /tmp/ctr/makerom && make deps >/dev/null && make -j4 >/dev/null \
 && install -m755 $(find . -type f -name makerom -perm -u+x | head -1) /opt/devkitpro/tools/bin/makerom \
 && rm -rf /tmp/ctr

ENV DEVKITPRO=/opt/devkitpro DEVKITARM=/opt/devkitpro/devkitARM
WORKDIR /src
