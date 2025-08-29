ARG REGISTRY_HOST
ARG BHF_CI_ARCH
FROM ${REGISTRY_HOST}/beckhoff/test-tcos/tc31-xar-um:bhf-13-trixie-${BHF_CI_ARCH}
ARG BHF_CI_ARCH
COPY tools/TwinCAT/3.1 /etc/TwinCAT/3.1
COPY PLC-TestProject/_Boot-${BHF_CI_ARCH} /etc/TwinCAT/3.1/Boot
