docker-images:
	docker build -t ucx .

test:
	docker-compose-up
