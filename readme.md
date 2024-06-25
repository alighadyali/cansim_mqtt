### How to use

- shell into the container `docker exec -it {containerid} /bin/bash`
- run `mosquitto` in one shell
- run `mosquitto_sub -h localhost -p 1883 -t testpub/topic -d` to check messages coming from the application
- run `mosquitto_pub -h localhost -p 1883 -t testsub/topic -m "{\"value1\":20,\"value2\":40}"` to send json to the application

- `docker build -t confobulater:arm64v8 -f Dockerfile.arm64v8.debug .`

docker build --rm -f "./Dockerfile.arm64v8.debug" -t confabulator:0.0.1-arm64v8 .
docker build --rm -f "./Dockerfile.arm64v8.debug" -t alighadyali/confabulator:0.0.1-arm64v8 .
docker push alighadyali/confabulator:0.0.1-arm64v8