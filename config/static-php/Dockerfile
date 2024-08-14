FROM webdevops/php-nginx:8.1-alpine

ARG HOST_UID
ARG HOST_GID

RUN usermod -o -u $HOST_UID application
RUN groupmod -o -g $HOST_GID application

COPY ./docker/php.ini /opt/docker/etc/php/php.ini
COPY ./docker/vhost.conf /opt/docker/etc/nginx/vhost.conf

WORKDIR /app
