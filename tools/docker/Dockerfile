FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive
ENV HOME=/home/big

VOLUME ["${HOME}"]
COPY entrypoint.sh /sbin/
RUN chmod 755 /sbin/entrypoint.sh
COPY build/src/metabasenet/metabasenet* /usr/bin/
COPY metabasenet.conf /metabasenet.conf
EXPOSE 6601 6602 6603 6604 6605
ENTRYPOINT ["/sbin/entrypoint.sh"]
CMD ["metabasenet"]
