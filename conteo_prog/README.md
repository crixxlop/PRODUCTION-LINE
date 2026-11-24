# Contador de cajas por color

Programa en C++17 que recibe las lecturas del sensor TCS3200 (enviadas por un
Arduino por el puerto serie), detecta cada caja que pasa, decide su color y
lleva un registro local en `registro_cajas.csv`.

Todo el programa esta en un solo archivo: `conteo_cajas.cpp`. Las clases
aparecen en este orden: datos basicos, fuentes, parser, clasificadores,
detector, repositorios, observadores, registro, configuracion, fabricas,
fachada y `main()`.

## Archivos

| Archivo | Para que sirve |
|---|---|
| `conteo_cajas.cpp` | todo el codigo del programa |
| `ejemplo_lecturas.txt` | lecturas de ejemplo para probar sin Arduino |
| `.gitignore` | evita subir el `.exe` y el `.csv` a Git |

## Compilar

```
g++ -std=c++17 -Wall conteo_cajas.cpp -o conteo_cajas.exe
```

## Ejecutar

```
conteo_cajas.exe --simulado
conteo_cajas.exe --archivo ejemplo_lecturas.txt
conteo_cajas.exe --serial COM3
conteo_cajas.exe --ayuda
```

El Arduino debe enviar lineas con el formato `R:45	V:190	A:160` a 9600 baudios.
Cierra el Monitor Serie del IDE de Arduino antes de usar `--serial`.

## Diseño

- Patrones: Strategy, Observer, Factory, Adapter, Repository, Facade.
- Principios SOLID y GRASP aplicados; la guia explica donde esta cada uno.
