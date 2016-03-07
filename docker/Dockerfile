# plda docker example
#
# VERSION 0.1

FROM ubuntu:14.04

# install mpich and ssh
RUN apt-get update && apt-get install -y \
    build-essential \
    mpich \
    openssh-server

RUN mkdir /var/run/sshd

# disable SSH host key checking 
RUN echo "StrictHostKeyChecking no" >> /etc/ssh/ssh_config

# SSH login fix.
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd

# generate an SSH key
RUN /usr/bin/ssh-keygen -f /root/.ssh/id_rsa -t rsa -N ''

# add its ssh keys to authorized_keys
RUN cp /root/.ssh/id_rsa.pub /root/.ssh/authorized_keys

RUN mkdir /root/plda

WORKDIR /root/plda

CMD ["/usr/sbin/sshd", "-D"]
