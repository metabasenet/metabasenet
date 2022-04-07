FROM ubuntu:18.04

ENV DEBIAN_FRONTEND noninteractive
ENV HOME=/home/big

VOLUME ["${HOME}"]
COPY entrypoint.sh /sbin/
RUN chmod 755 /sbin/entrypoint.sh
COPY build/src/metabasenet/metabasenet* /usr/bin/
COPY metabasenet.conf /metabasenet.conf
EXPOSE 8811 8812 8813 8814 8815
ENTRYPOINT ["/sbin/entrypoint.sh"]
CMD ["metabasenet"]
