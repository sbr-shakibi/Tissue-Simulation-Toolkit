# Dockerfile for Tissue Simulation Toolkit

1. set up ssh access for github

2. locally activate a X-Window-Server (XQuartz, X11)
xhost + ${hostname}

3. Build the Docker image in the folder where the Dockerfile is located
docker build --ssh default -t tst2-docker:latest .  

4. Run the Docker container with a X11-Server
docker run -it \
  --env DISPLAY=host.docker.internal:0 \
  --volume /tmp/.X11-unix:/tmp/.X11-unix \
  --user root tst2-docker:latest