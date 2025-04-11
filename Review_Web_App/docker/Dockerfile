FROM ubuntu:jammy

# Agent forwarding during docker build https://stackoverflow.com/questions/43418188/ssh-agent-forwarding-during-docker-build
# install ssh client and git
RUN apt-get update
RUN apt-get install -y openssh-client git

# download public key for github.com
RUN mkdir -p -m 0600 ~/.ssh && ssh-keyscan github.com >> ~/.ssh/known_hosts

# Install tools
RUN apt-get update &&  apt-get install -y \
    nano \
    ranger \
    tmux \
    tmuxinator \
    htop

# Copy tools configuration
COPY ./to_copy/nanorc /root/.nanorc
COPY ./to_copy/ranger /root/.config/ranger/rc.conf
COPY ./to_copy/aliases /root/.bash_aliases
COPY ./to_copy/tmux /root/.tmux.conf
WORKDIR /root

RUN --mount=type=ssh git clone git@github.com:WatchPlant/WP_network.git

RUN apt-get install -y \
    build-essential \
    python3-pip \
    gunicorn

RUN python3 -m pip install \
    pyyaml \
    dash \
    dash-bootstrap-components \
    paho-mqtt

# Add autocomplete
RUN echo "\
if ! shopt -oq posix; then \n\
  if [ -f /usr/share/bash-completion/bash_completion ]; then \n\
    . /usr/share/bash-completion/bash_completion \n\
  elif [ -f /etc/bash_completion ]; then \n\
    . /etc/bash_completion \n\
  fi \n\
fi" >> $HOME/.bashrc
