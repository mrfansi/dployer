FROM webdevops/php-nginx:8.1-alpine

RUN apk --no-cache add coreutils

ENV NODE_PACKAGE_URL  https://unofficial-builds.nodejs.org/download/release/v18.16.0/node-v18.16.0-linux-x64-musl.tar.gz

RUN apk add libstdc++
WORKDIR /opt
RUN wget $NODE_PACKAGE_URL
RUN mkdir -p /opt/nodejs
RUN tar -zxvf *.tar.gz --directory /opt/nodejs --strip-components=1
RUN rm *.tar.gz
RUN ln -s /opt/nodejs/bin/node /usr/local/bin/node
RUN ln -s /opt/nodejs/bin/npm /usr/local/bin/npm
RUN npm install -g yarn

ARG HOST_UID
ARG HOST_GID

COPY ./docker/laravel-init.sh /opt/docker/provision/entrypoint.d/laravel-init.sh
COPY ./docker/php.ini /opt/docker/etc/php/php.ini
COPY ./docker/vhost.conf /opt/docker/etc/nginx/vhost.conf
COPY ./docker/queue.conf /opt/docker/etc/supervisor.d/queue.conf
COPY ./docker/cron /opt/docker/etc/cron/application
COPY ./docker/cron.sh /opt/docker/bin/service.d/cron.d/10-init.sh

RUN usermod -o -u $HOST_UID application
RUN groupmod -o -g $HOST_GID application

WORKDIR /app
