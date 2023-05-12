# malloc
Lugar para respuestas en prosa y documentación del TP.

Se eligio como tamaño minimo de region 40 bytes, ya que es el tamaño de un header de la misma. De esta forma los datos guardados ocupan como minimo lo mismo que el header, y no se gasta mas memoria en el header de los metadatos que los datos en si.

Al realizar un free sobre una region ya liberada, se printea un mensaje de error y se setea la variable errno a ENOMEM. Esto se hace para que el programa que llamo a free sepa que hubo un error y pueda actuar en consecuencia.
