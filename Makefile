ucx-py:
	git clone https://github.com/Akshay-Venkatesh/ucx-py

ucx:
	git clone https://github.com/openucx/ucx

docker-images: Dockerfile
	nvidia-docker build -t ucx .

test:
	docker-compose-up

dev:
	docker run -v $$(pwd)/ucx-py:/ucx-py --name=ucx-dev --rm -it ucx bash
