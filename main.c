#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/device.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Pedro Henrique e Fabio");
MODULE_DESCRIPTION("Lista ordenada");
MODULE_VERSION("1.0");

#define DEVICE_NAME "lista_random"
#define CLASS_NAME  "lista_class"
#define MAX_ELEM     100

static int major;
static struct class *lista_class = NULL;
static struct device *lista_device = NULL;

static int lista[MAX_ELEM];
static int count = 0;

/*
 * Bubble sort
 */
static void ordenar_lista(void)
{
    int i, j, tmp;
    for (i = 0; i < count - 1; i++)
        for (j = 0; j < count - i - 1; j++)
            if (lista[j] > lista[j + 1]) {
                tmp = lista[j];
                lista[j] = lista[j + 1];
                lista[j + 1] = tmp;
            }
}

/*
 * WRITE — recebe numero e coloca na lista
 */
static ssize_t dev_write(struct file *file, const char __user *buf,
                         size_t len, loff_t *offset)
{
    char kbuf[32];
    int valor;

    if (len > 31)
        len = 31;

    if (copy_from_user(kbuf, buf, len))
        return -EFAULT;

    kbuf[len] = '\0';

    if (kstrtoint(kbuf, 10, &valor) != 0)
        return -EINVAL;

    if (count >= MAX_ELEM)
        return -ENOMEM;

    lista[count++] = valor;
    ordenar_lista();

    printk(KERN_INFO "[lista_random] Recebido: %d (total: %d)\n",
           valor, count);

    return len;
}

/*
 * READ — imprime lista, menor e maior
 */
static ssize_t dev_read(struct file *file, char __user *buf,
                        size_t len, loff_t *offset)
{
    char out[1024];
    int pos = 0, i;

    if (*offset != 0)
        return 0;

    if (count == 0)
        pos += snprintf(out + pos, sizeof(out) - pos, "Lista vazia\n");
    else {
        pos += snprintf(out + pos, sizeof(out) - pos,
                        "Lista ordenada (%d valores): ", count);

        for (i = 0; i < count; i++)
            pos += snprintf(out + pos, sizeof(out) - pos, "%d ", lista[i]);

        pos += snprintf(out + pos, sizeof(out) - pos,
                        "\nMenor: %d\nMaior: %d\n",
                        lista[0], lista[count - 1]);
    }

    if (copy_to_user(buf, out, pos))
        return -EFAULT;

    *offset = pos;
    return pos;
}

static struct file_operations fops = {
    .owner = THIS_MODULE,
    .read  = dev_read,
    .write = dev_write
};

/*
 * INIT — registra driver e cria /dev/lista_random
 */
static int __init lista_init(void)
{
    major = register_chrdev(0, DEVICE_NAME, &fops);
    if (major < 0)
        return major;

    // Apara kernel 6 do linux pra evitar o erro chato na hora do make
    lista_class = class_create(CLASS_NAME);
    if (IS_ERR(lista_class)) {
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(lista_class);
    }

    lista_device = device_create(lista_class, NULL, MKDEV(major, 0),
                                 NULL, DEVICE_NAME);
    if (IS_ERR(lista_device)) {
        class_destroy(lista_class);
        unregister_chrdev(major, DEVICE_NAME);
        return PTR_ERR(lista_device);
    }

    printk(KERN_INFO "[lista_random] Driver criado automaticamente\n");
    return 0;
}

/*
 * EXIT — remove driver e destroi /dev
 */
static void __exit lista_exit(void)
{
    device_destroy(lista_class, MKDEV(major, 0));
    class_destroy(lista_class);
    unregister_chrdev(major, DEVICE_NAME);

    printk(KERN_INFO "[lista_random] Removido\n");
}

module_init(lista_init);

module_exit(lista_exit);
