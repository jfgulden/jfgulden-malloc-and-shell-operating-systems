# shell

### Búsqueda en $PATH
¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?
-La diferencia es que execve ejecuta el programa cuya ruta le es pasada por parámetros, esto hace que el programa que está siendo corrido por el proceso que llama a la función sea completamente reemplazado por este otro, siéndole asignados un stackframe y un heap nuevos.
Por el contrario, la familia de wrappers exec modifica la imagen del proceso que hace el llamado, reemplazándola por la imagen del proceso del archivo ejecutable. Son como el front-end de la syscall execve y proporcionan distintas interfaces para luego ejecutar la syscall execve.

¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?
-Sí, la llamada a exec puede fallar por varios motivos. Algunos de ellos son:
	1) Porque el permiso para abrir el archivo es negado y no hay ningún otro que pueda ser ejecutado.
	2) Porque el header o el archivo no se reconocen o no existen.
En estos casos, la syscall retorna -1 y el proceso que hizo el llamado, continúa con su ejecución.
En la implementación de la shell, cuando esto ocurre, se maneja el error y se le comunica al usuario por stdout que el comando ingresado no existe o que ocurrió un error al intentar ejecutarlo.

### Procesos en segundo plano

Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.
-Para implementar procesos en segundo plano primero se debe crear un proceso hijo que ejecute el comando ingresado cuya sintaxis contiene el caractér '&'.
Una vez hecho esto, el proceso padre (es decir la shell) deberá esperar oportunamente al proceso hijo a que termine con la ejecución. Esto quiere decir que lo esperará si terminó de ejecutarse, pero continuará con su ejecución si no lo hizo. 
En caso de que haya terminado su ejecucion, funciona como haber ejecutado el comando en primer plano, es decir, se muestra por stdout la salida del comando y se devuelve el prompt de la shell.
Luego de esperarlo, en caso de que no haya terminado su ejecución, se devolverá el prompt de la shell, mediante el cual podremos seguir ejecutando comandos sin afectar la ejecución del anterior. Es por esto que se los denominan "Procesos en segundo plano", 
	porque se pueden seguir ejecutando otros procesos y no van a alterar la ejecución del proceso en segundo plano.
En nuestra implementación utilizamos la syscall waitpid, la cual será ejecutada por la shell cada vez que enviemos algo por stdin a la misma, sea o no un comando. Esto junto al flag WNOHANG permite esperar oportunamente al proceso hijo, y si no terminó de ejecutarse, seguir con la ejecución de la shell.
Una vez que el proceso en segundo plano haya terminado de ejecutarse, si enviamos algo por stdin a la shell, se mostrará la salida de éste. Esto sucede porque el nuevo proceso espera oportunísticamente al proceso en segundo plano y, al haber terminado de ejecutarse, espera a que éste muestre su resultado por stdout antes de seguir con la ejecución.

### Flujo estándar

Responder: Investigar el significado de 2>&1, explicar cómo funciona su forma general
Mostrar qué sucede con la salida de cat out.txt en el ejemplo.
Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).

-2>&1 es un operador de redireccionamiento que se utiliza para redirigir la salida de error estándar (stderr) a la misma ubicación que la salida estándar (stdout).
Lo que sucede con la salida de cat out.txt en el ejemplo es que en la línea anterior, se redirige la salida stdout al archivo out.txt, pero al no haber estado creado el archivo, se crea para guardar la salida del programa. Además, se redirige la salida del error estándar al archivo al que apunta el file descriptor 1 (que antes correspondía a la salida estándar). Como ese archivo es out.txt, entonces se guarda la salida, que en este caso es un error en out.txt.
Debido a esto, al hacer cat out.txt se va a mostrar por pantalla el error que surgió de ejecutar la línea anterior.
Al repetirlo invirtiendo el orden de las redirecciones, lo que sucede es que se redirige el file descriptor de la salida del error estándar a la salida estándar, y luego se redirige el file descriptor de la salida estándar al archivo out.txt. Por lo tanto, se mostrará por pantalla el error y se escribirá en el archivo out.txt la salida estándar (que no contiene el error) producto de la ejecución de la línea.

### Tuberías múltiples
Responder: Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe
¿Cambia en algo?


¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.
-Si alguno de los comandos falla, el otro comando se ejecuta normalmente. En caso de que el comando de la derecha deba recibir el output del extremo izquierdo para poder ejecutarse, estaría recibiendo un error, por lo que no se ejecutaría adecuadamente.
Ejemplos en bash:
$ noexiste | echo hola
hola
noexiste: command not found

$ noexiste | cat
noexiste: command not found

En nuestra implementacion:
$ noexiste | echo hola
hola
fallo al ejecutar el comando: No such file or directory

$ noexiste | cat
fallo al ejecutar el comando: No such file or directory

### Variables de entorno temporarias
Responder: ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?
-Es necesario hacerlo luego de la llamada al fork porque las variables de entorno temporales son asignadas únicamente al proceso hijo y no se puede acceder a ellas desde el proceso padre (que es la shell), de ahí el nombre temporales. Por lo tanto, si se intenta acceder a ellas desde el proceso padre, se obtendrá un error.

Responder: En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).
¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.
-El comportamiento no es el mismo porque en lugar de setear las variables de entorno para el proceso que llama a la función, las setea para el nuevo proceso que se va a ejecutar. 

### Pseudo-variables
Responder: Investigar al menos otras tres variables mágicas estándar, y describir su propósito.
'$$': El ID de proceso (PID) del shell actual, nos sirve para identificar el proceso que se está ejecutando.
'$!': El ID de proceso (PID) del último proceso ejecutado en segundo plano, nos sirve para identificar el proceso que se está ejecutando en segundo plano.
'$0': El nombre del shell o del script actual, nos sirve para identificar el nombre del shell o del script que se está ejecutando.
Ejemplos de uso en bash:
$ echo $$
1228
$ ps 1228
  PID TTY      STAT   TIME COMMAND
 1228 pts/0    S      0:00 bash

$ sleep 10 &
[1] 1240
$ echo $!
1240

$ echo $0
bash
---

### Comandos built-in
Responder: ¿Entre cd y pwd, alguno de los dos se podría implementar sin necesidad de ser built-in? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como built-in? (para esta última pregunta pensar en los built-in como true y false)
-El comando cd al cambiar el directorio donde esta funcionando la shell, no se puede implementar sin ser built-in, por este motivo se realiza antes del fork, debido a que es un proceso que afecta a la shell en si. No se puede implementar cd como un comando externo porque los comandos externos se ejecutan en su propio proceso y, por lo tanto, no pueden cambiar el directorio de trabajo actual de la shell que los llama. El comando pwd por otro lado al mostrar el directorio donde esta funcionando la shell, se puede implementar sin ser built-in, ya que no modifica la shell en si.
El motivo de hacerlo como built-in es que 'pwd' es un comando que se utiliza muy seguido y es mas eficiente que sea built-in ya que se ahorra tiempo en la creacion de un proceso nuevo, y en el parseo del mismo, como revisamos si un comando es built-in o no antes de parsearlo, si es built-in no se parsea y se ejecuta directamente.
---

### Historial
Responder: ¿Cuál es la función de los parámetros MIN y TIME del modo no canónico? ¿Qué se logra en el ejemplo dado al establecer a MIN en 1 y a TIME en 0?
-Dentro del modo no canónico hay definidas dos variables, TIME y MIN, que definen cuatro modos de funcionamiento diferentes dependiendo de su valor:
TIME=0 y MIN=0: la lectura vuelve de inmediato. Lee caracteres si los hay y, si no, read devuelve 0.
TIME=0 y MIN>0: la lectura se queda bloqueada hasta que haya MIN caracteres al menos que leer.
TIME>0 y MIN=0: la lectura se queda bloqueada un máximo de TIME décimas de segundo hasta que haya algún carácter que leer.
TIME>0 y MIN>0: ha de haber al menos MIN caracteres para que read vuelva y volverá también si la pulsación entre dos de ellos supera TIME décimas de segundo.
MIN especifica el número mínimo de caracteres que deben ser ingresados antes de que se envíe la entrada a la aplicación que la está esperando.
TIME especifica el tiempo máximo en décimas de segundo que se esperará para que el usuario ingrese el siguiente carácter antes de que se envíe la entrada a la aplicación.
En el ejemplo dado al establecer MIN en 1 y TIME en 0, la entrada del usuario se enviará a la aplicación tan pronto como se ingrese el primer carácter, sin esperar ningún tiempo adicional. 
---
