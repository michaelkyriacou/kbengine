docker-down:
	docker-compose down

docker-build:
	docker-compose build

docker-run: docker-down docker-build
	docker-compose up