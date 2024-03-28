#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

#define DEVICE_NAME "chardev"
#define BUF_LEN 80 

static int major;
static char msg[BUF_LEN + 1];

static struct class *cls;

// Prototipos de las funciones
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);

static struct file_operations chardev_fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release,
};

// Función que se ejecuta al cargar el módulo
int init_module(void) {
    // Se registra el dispositivo de caracteres
    major = register_chrdev(0, DEVICE_NAME, &chardev_fops);
    if (major < 0) {
        pr_alert("Falló el registro del dispositivo de caracteres con el número mayor %d\n", major);
        return major;
    }
    pr_info("Dispositivo registrado con el número mayor %d.\n", major);
    
    // Se crea la clase del dispositivo
    cls = class_create(THIS_MODULE, DEVICE_NAME);
    // Se crea el dispositivo en /dev
    device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

    pr_info("Dispositivo creado en /dev/%s\n", DEVICE_NAME);

    return 0;
}

// Función que se ejecuta al descargar el módulo
void cleanup_module(void) {
    // Se borra el dispositivo
    device_destroy(cls, MKDEV(major, 0));
    // Se borra la clase del dispositivo
    class_destroy(cls);
    // Se desregistra el dispositivo de caracteres
    unregister_chrdev(major, DEVICE_NAME);

    pr_info("Dispositivo desregistrado\n");
}

// Función que se ejecuta al abrir el dispositivo
static int device_open(struct inode *inode, struct file *file) {
    // Incrementa el contador de uso del módulo
    try_module_get(THIS_MODULE);
    return 0;
}

// Función que se ejecuta al cerrar el dispositivo
static int device_release(struct inode *inode, struct file *file) {
    // Decrementa el contador de uso del módulo
    module_put(THIS_MODULE);
    return 0;
}

// Función que se ejecuta al leer desde el dispositivo
static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) {
    int bytes_read = 0;
    int msg_len = strlen(msg);
    
    // Si se leyó hasta el final del mensaje, se retorna 0
    if (*offset >= msg_len)
        return 0;
    
    // Calcula la cantidad de bytes a leer
    bytes_read = min(length, (size_t)(msg_len - *offset));

    // Copia el mensaje al espacio de usuario
    if (copy_to_user(buffer, msg + (msg_len - *offset - bytes_read), bytes_read))
        return -EFAULT;

    *offset += bytes_read;

    return bytes_read;
}

// Función que se ejecuta al escribir en el dispositivo
static ssize_t device_write(struct file *filp, const char __user *buff, size_t len, loff_t *off) {
    int bytes_written = 0;
    int start = 0;
    int end = len - 1;
    char temp;

    if (len > BUF_LEN) {
        pr_alert("El texto es demasiado largo.\n");
        return -EINVAL;
    }

    // Copia el texto desde el espacio de usuario al buffer del módulo
    if (copy_from_user(msg, buff, len)) {
        pr_alert("Error al copiar desde el espacio de usuario.\n");
        return -EFAULT;
    }

    pr_info("Texto recibido: %s\n", msg);

    // Evito invertir el carácter de salto de línea
    if (msg[end] == '\n') {
        end--;
    }

    // Invierte el texto
    while (start < end) {
        temp = msg[start];
        msg[start] = msg[end];
        msg[end] = temp;
        start++;
        end--;
    }

    bytes_written = len;
    pr_info("Texto invertido: %s\n", msg);

    return bytes_written;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Giovanni Sia");
MODULE_DESCRIPTION("Módulo del kernel de dispositivo de caracteres que permite la escritura de texto y al leer muestra el texto invertido");
