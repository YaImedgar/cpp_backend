docker build -t my_http_server $(dirname "$0") && \
docker run --rm -p 80:8080 my_http_server
