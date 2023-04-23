# shell

### Búsqueda en $PATH
¿cuáles son las diferencias entre la syscall execve(2) y la familia de wrappers proporcionados por la librería estándar de C (libc) exec(3)?
La diferencia es que execve ejecuta el programa cuya ruta le es pasada por parámetros, esto hace que el programa que está siendo corrido por el proceso que llama a la función sea completamente reemplazado por este otro, siéndole asignados un stackframe y un heap nuevos.
Por el contrario, la system call exec no reemplaza el programa por completo, sino que modifica únicamente la imagen del proceso que hace el llamado, reemplazándola por la imagen del proceso del archivo ejecutable. 
Cabe aclarar que la familia de wrappers exec pueden ejecutar un programa o un archivo (que contenga por ejemplo un comando), mientras que la syscall execve solo puede ejecutar un programa.

¿Puede la llamada a exec(3) fallar? ¿Cómo se comporta la implementación de la shell en ese caso?
Sí, la llamada a exec puede fallar por varios motivos. Algunos de ellos son:
	1) Porque el permiso para abrir el archivo es negado y no hay ningún otro que pueda ser ejecutado.
	2) Porque el header o el archivo no se reconocen o no existen.
En estos casos, la syscall retorna -1 y el proceso que hizo el llamado, continúa con su ejecución.
En la implementación de la shell, cuando esto ocurre, se maneja el error y se le comunica al usuario por stdout que el comando ingresado no existe o que ocurrió un error al intentar ejecutarlo.


### Procesos en segundo plano

Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.
Para implementar procesos en segundo plano primero se debe crear un proceso hijo que ejecute el comando ingresado cuya sintaxis contiene el caractér '&'.
Una vez hecho esto, el proceso padre deberá esperar oportunamente al proceso hijo a que termine con la ejecución. Esto quiere decir que lo esperará si terminó de ejecutarse, pero continuará con su ejecución si no lo hizo. 
Luego de esperarlo, en caso de que no haya terminado su ejecución, se devolverá el prompt de la shell, mediante el cual podremos seguir ejecutando comandos sin afectar la ejecución del anterior. Es por esto que se los denominan "Procesos en segundo plano", 
	porque se pueden seguir ejecutando otros procesos y no van a alterar la ejecución del proceso en segundo plano.
En nuestra implementación utilizamos la syscall waitpid, la cual será ejecutada por el nuevo proceso cada vez que enviemos algo por stdin a la shell, sea o no un comando.
Una vez que el proceso en segundo plano haya terminado de ejecutarse, si enviamos algo por stdin a la shell, se mostrará la salida de éste. Esto sucede porque el nuevo proceso espera oportunísticamente al proceso en segundo plano y, al haber terminado de ejecutarse, espera a que éste muestre su resultado por stdout antes de seguir con la ejecución.


### Flujo estándar



Responder: Investigar el significado de 2>&1, explicar cómo funciona su forma general
Mostrar qué sucede con la salida de cat out.txt en el ejemplo.
Luego repetirlo, invirtiendo el orden de las redirecciones (es decir, 2>&1 >out.txt). ¿Cambió algo? Compararlo con el comportamiento en bash(1).
2>&1 es un operador de redireccionamiento que se utiliza para redirigir la salida de error estándar (stderr) a la misma ubicación que la salida estándar (stdout).
Lo que sucede con la salida de cat out.txt en el ejemplo es que en la línea anterior, se redirige la salida stdout al archivo out.txt, pero al no haber estado creado el archivo, se crea para guardar la salida del programa. Además, se redirige la salida del error estándar al archivo al que apunta el file descriptor 1 (que antes correspondía a la salida estándar). Como ese archivo es out.txt, entonces se guarda la salida, que en este caso es un error en out.txt.
Debido a esto, al hacer cat out.txt se va a mostrar por pantalla el error que surgió de ejecutar la línea anterior.
Al repetirlo invirtiendo el orden de las redirecciones, lo que sucede es que se redirige el file descriptor de la salida del error estándar a la salida estándar, y luego se redirige el file descriptor de la salida estándar al archivo out.txt. Por lo tanto, se mostrará por pantalla el error y se escribirá en el archivo out.txt la salida estándar (que no contiene el error) producto de la ejecución de la línea.

### Tuberías múltiples
Responder: Investigar qué ocurre con el exit code reportado por la shell si se ejecuta un pipe
¿Cambia en algo?
¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando bash. Comparar con su implementación.

Si alguno de los comandos falla, el otro comando se ejecuta normalmente. En caso de que el comando de la derecha deba recibir el output del extremo izquierdo para poder ejecutarse, estaría recibiendo un error, por lo que tampoco se ejecutaría y terminaría enviando por stdout su propio error.

### Variables de entorno temporarias

Responder: ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?
Responder: En algunos de los wrappers de la familia de funciones de exec(3) (las que finalizan con la letra e), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar setenv(3) por cada una de las variables, se guardan en un arreglo y se lo coloca en el tercer argumento de una de las funciones de exec(3).
¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.
Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.

Es necesario hacerlo luego de la llamada al fork porque las variables de entorno temporales son asignadas únicamente al proceso hijo (de ahí el nombre temporales) y no se puede acceder a ellas desde el proceso padre.
El comportamiento no es el mismo porque en lugar de setear las variables de entorno para el proceso que llama a la función, las setea para el nuevo proceso que se va a ejecutar. 

### Pseudo-variables

---

### Comandos built-in

---

### Historial

---
