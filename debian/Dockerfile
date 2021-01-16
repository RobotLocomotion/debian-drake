FROM ubuntu:20.04
ENV LANG C.UTF-8
ENV LOGNAME root
ENV USER root
RUN export DEBIAN_FRONTEND=noninteractive \
  && apt-get update -qq \
  && apt-get install --no-install-recommends -qy \
    autopkgtest \
    build-essential \
    ca-certificates \
    equivs \
    git-buildpackage \
    gnupg \
    lintian \
    pristine-tar \
    xz-utils \
  && export APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=1 \
  && apt-key adv --fetch-keys -q https://bazel.build/bazel-release.pub.gpg \
  && echo 'deb [arch=amd64] https://storage.googleapis.com/bazel-apt stable jdk1.8' \
    > /etc/apt/sources.list.d/bazel.list \
  && rm -rf /var/lib/apt/lists/* \
  && echo .DS_Store > /root/.gitignore \
  && git config --global core.excludesfile /root/.gitignore \
  && git config --global tar.tar.xz.command "xz -c"
COPY control /tmp
RUN export DEBIAN_FRONTEND=noninteractive \
  && apt-get update -qq \
  && mk-build-deps -it "apt-get --no-install-recommends -qy" /tmp/control \
  && rm -rf /tmp/control /var/lib/apt/lists/*
