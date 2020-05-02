all: push

build:
	docker build . -t growthteam/daq-ci-ubuntu
push: build
	docker login
	docker push growthteam/daq-ci-ubuntu