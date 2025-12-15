# game_server_backend
Это backend-сервер для многопользовательской игры.  
Игроки управляют собакой, задача которой — находить и собирать потерянные вещи.

Цель игры — набрать как можно больше очков. Очки начисляются за доставку предметов
в бюро находок. Предметы отличаются своей **стоимостью**. По умолчанию игрок может нести одновременно до **3 предметов**.

# Сборка под Linux

 * **Release:**
```bash
mkdir /app/build
cd /app/build
conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Release
cmake -DCMAKE_BUILD_TYPE=Release
cmake --build .
```
 * **Debug:**
```bash
mkdir /app/build
cd /app/build
conan install .. --build=missing -s compiler.libcxx=libstdc++11 -s build_type=Debug
cmake -DCMAKE_BUILD_TYPE=Debug
cmake --build .
```
