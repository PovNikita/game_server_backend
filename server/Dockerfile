#Создать образ на основе базового слоя gcc (там будет ОС и компилятор)
#11.3 - используемая версия gcc
FROM gcc:11.3 AS build

#Выполнить установку зависимостей внутри контейнера
RUN apt update \
    && \
    apt upgrade -y \
    && \
    apt install -y \
      python3-pip \
      cmake \
    && \
    pip3 install conan==1.*

#Собрать boost
COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
        conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Debug     


#Скопировать файлы проекта внутрь контейнера
COPY ./src /app/src
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

#Собрать и запустить проект
RUN cd /app/build && \
        cmake -DCMAKE_BUILD_TYPE=Debug .. && \
        cmake --build .

#Контейнер для запуска
FROM ubuntu:22.04 AS run
RUN groupadd -r www && useradd -r -g www www
USER www

#Копируем с контейнера build приложение
COPY --from=build /app/build/game_server /app/
COPY --from=build /app/build/game_server_test /app/
#Копируем папку с конфигурационными файлами для работы приложения
COPY ./data /app/data
COPY ./static /app/static

#Запуск приложения
ENTRYPOINT ["/app/game_server", "--config-file", "/app/data/config.json", "--www-root", "/app/static"]


