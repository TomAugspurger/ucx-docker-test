ucx-py:
	git clone https://github.com/Akshay-Venkatesh/ucx-py

ucx:
	git clone https://github.com/openucx/ucx

docker-images: Dockerfile
	docker build -t ucx .

test:
	docker-compose-up
