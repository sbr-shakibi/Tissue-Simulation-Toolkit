# Tissue Simulation Toolkit - Docker Setup

This guide describes how to build and run the Tissue Simulation Toolkit (`tst2-docker`) with Docker, including X11 GUI support and SSH access to GitHub.

---

## Prerequisites

* Docker
* X11 server (e.g., [XQuartz](https://www.xquartz.org/) on macOS, X11 on Linux)
* SSH keys configured for GitHub access

---

## 1. SSH Access to GitHub

Ensure your SSH key is added to the SSH agent so Docker can access GitHub during the build:

```bash
ssh-add ~/.ssh/id_rsa
```

---

## 2. Enable X11 Forwarding

Start your X11 server and allow connections from your local machine:

```bash
xhost + ${hostname}
```

> Replace `${hostname}` with the actual hostname of your machine, or use `localhost` if unsure.

---

## 3. Build the Docker Image

Navigate to the folder containing the Dockerfile and build the image:

```bash
docker build --ssh default -t tst2-docker:latest .
```

---

## 4. Run the Docker Container with X11 Support

```bash
docker run -it \
  --env DISPLAY=host.docker.internal:0 \
  --volume /tmp/.X11-unix:/tmp/.X11-unix \
  --user root tst2-docker:latest
```

This command starts the container with GUI support for visualization tools.

---

## 5. Run the Simulation

Inside the Docker container:

```bash
cd bin
./vessel ../data/chemotaxis.par
```

To activate the Python virtual environment and run the simulation manager:

```bash
. venv/bin/activate
muscle_manager --start-all ymmsl/adhesions.ymmsl ymmsl/plot_state.ymmsl
```

---

## Notes

* Make sure `XQuartz` or another X11 server is running and accepts connections before launching the Docker container.
* If you encounter display issues on macOS, ensure `XQuartz` has “Allow connections from network clients” enabled in preferences.

---
