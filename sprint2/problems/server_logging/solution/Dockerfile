# Создать образ на основе базового слоя gcc (там будет ОС и сам компилятор).
# 11.3 — используемая версия gcc.
FROM gcc:11.3 AS build

# Выполнить установку зависимостей внутри контейнера.
RUN apt update && \
  apt install -y \
  python3-pip \
  cmake
RUN pip3 install "conan>=1.59,<2.0"


# копируем conanfile.txt в контейнер и запускаем conan install
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
  conan install .. --build=missing -s build_type=Release -s compiler.libcxx=libstdc++11

# только после этого копируем остальные иходники
COPY CMakeLists.txt /app/
COPY ./include /app/include
COPY ./src /app/src

RUN cd /app/build && \
  cmake -DCMAKE_BUILD_TYPE=Release .. && \
  cmake --build .

# Второй контейнер в том же докерфайле
FROM ubuntu:22.04 AS run

# Создадим пользователя www
RUN groupadd -r www && useradd -r -g www www
USER www

# Скопируем приложение со сборочного контейнера в директорию /app.
# Не забываем также папку data и static, она пригодится.
COPY --from=build /app/build/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

# Запускаем игровой сервер
ENTRYPOINT ["/app/game_server", "/app/data/config.json", "/app/static/"] 
