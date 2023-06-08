FROM espressif/idf:release-v5.1

ARG USERNAME=idf
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN echo ". $IDF_PATH/export.sh" >> /etc/bash.bashrc

# Fix "locale.Error: unsupported locale setting" when running `idf.py menuconfig`
RUN echo 'export LC_ALL=C' >> /etc/bash.bashrc
RUN echo 'export LC_CTYPE="en_US.UTF-8"' >> /etc/bash.bashrc

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME -s /bin/bash

RUN usermod -aG dialout $USERNAME

USER $USERNAME

# /opt/esp/idf is owned by root but we work as a non root user
RUN git config --global --add safe.directory /opt/esp/idf
