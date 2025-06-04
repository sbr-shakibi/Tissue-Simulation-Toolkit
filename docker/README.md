# Tissue Simulation Toolkit - Docker Setup

This guide describes how to build and run the Tissue Simulation Toolkit with Docker (`tst2-docker`), including X11 GUI support and SSH access to GitHub.

## Prerequisites

* [Docker](https://docker.com)
* X11 server (e.g., [XQuartz](https://www.xquartz.org/) on macOS, X11 on Linux)
* SSH keys configured for GitHub access

## 1. SSH Access to GitHub

Ensure your SSH key is added to the SSH agent so Docker can access GitHub during the build:

```bash
ssh-add ~/.ssh/id_rsa
```

## 2. Enable X11 Forwarding

Start your X11 server and allow connections from your local machine:

```bash
xhost + ${hostname}
```

> You might have to replace `${hostname}` with the actual hostname of your machine or you can try `localhost` and `local:docker`. 

## 3. Build the Docker Image

Navigate to the folder containing the Dockerfile and build the image:

```bash
DOCKER_BUILDKIT=1 docker build --ssh default --platform linux/amd64 -t tst2-docker:latest .
```

> On some machines you can omit `DOCKER_BUILDKIT=1` because this is set as default. In case you get the error `unknown flag --ssh` while running `docker build` with the preamble `DOCKER_BUILDKIT=1` it could mean you are using a Docker version older than 18.09. The `--ssh` flag was introduced as part of the Docker BuildKit, which only works in newer Docker versions.

## 4. Run the Docker Container with X11 Support

```bash
docker run -it \
  --env DISPLAY=host.docker.internal:0 \
  --volume /tmp/.X11-unix:/tmp/.X11-unix \
  --user root tst2-docker:latest
```

This command starts the container with GUI support for visualization tools.

> In case you had to use `local:docker` as hostname in step 2, try `--env DISPLAY=$DISPLAY` if step 5 fails to connect to X display.

## 5. Run standalone Cellular Potts Model (TST)

Inside the Docker container:

```bash
cd bin
./vessel ../data/chemotaxis.par
```

## 6. Run Cellular Potts Model with Extra-Cellular Matrix (TST-MD)

To activate the Python virtual environment and run the simulation manager:

```bash
. venv/bin/activate
muscle_manager --start-all ymmsl/adhesions.ymmsl ymmsl/plot_state.ymmsl
```

## Notes

* Make sure `XQuartz` or another X11 server is running and accepts connections before launching the Docker container.
* If you encounter display issues on macOS, ensure `XQuartz` has “Allow connections from network clients” enabled in preferences.
* When running `docker build` on an Apple Silicon Mac, the flag `--platform linux/amd64` ensures Docker builds for the Intel/AMD architecture (amd64). Without this, Docker defaults to arm64, which may not be fully supported by all dependencies or the Makefile, potentially causing build failures.