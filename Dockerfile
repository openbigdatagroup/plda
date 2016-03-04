FROM ubuntu:14.04

RUN apt-get update && apt-get install -y \
    build-essential \
    mpich \
    openssh-server

RUN mkdir /var/run/sshd

RUN echo "StrictHostKeyChecking no" >> /etc/ssh/ssh_config

RUN /usr/bin/ssh-keygen -f /root/.ssh/id_rsa -t rsa -N ''

RUN cp /root/.ssh/id_rsa.pub /root/.ssh/authorized_keys

RUN mkdir /root/plda

WORKDIR /root/plda

CMD ["/usr/sbin/sshd", "-D"]
