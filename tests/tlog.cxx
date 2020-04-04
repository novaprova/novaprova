#include "np/util/log.hxx"

static void simple_test(void)
{
    fprintf(stderr, "simple_test\n");
    np::log::basic_config(np::log::DEBUG, NULL);
    dprintf("small batch, at DEBUG level, should appear.");
    iprintf("twee, at INFO level, should appear.");
    wprintf("meh, at WARN level, should appear.");
    eprintf("fingerstache, at ERROR level, should appear.");
}

static void level_test(void)
{
    fprintf(stderr, "level_test\n");

    fprintf(stderr, "setting INFO level\n");
    np::log::basic_config(np::log::INFO, NULL);
    dprintf("typewriter, at DEBUG level, should not appear");
    iprintf("meditation, at INFO level, should appear");
    wprintf("tumeric, at WARN level, should appear");
    eprintf("tattooed, at ERROR level, should appear");

    fprintf(stderr, "setting WARN level\n");
    np::log::basic_config(np::log::WARN, NULL);
    dprintf("kombucha, at DEBUG level, should not appear");
    iprintf("migas, at INFO level, should not appear");
    wprintf("shaman, at WARN level, should appear");
    eprintf("selfies, at ERROR level, should appear");

    fprintf(stderr, "setting ERROR level\n");
    np::log::basic_config(np::log::ERROR, NULL);
    dprintf("williamsburg, at DEBUG level, should not appear");
    iprintf("waistcoat, at INFO level, should not appear");
    wprintf("unicorn, at WARN level, should not appear");
    eprintf("lyft, at ERROR level, should appear");
}

static void newline_test(void)
{
    fprintf(stderr, "newline_test\n");
    np::log::basic_config(np::log::DEBUG, NULL);
    eprintf("distillery, has no trailing newline");
    eprintf("taxidermy, has a trailing newline\n");
    eprintf("raclette, contains\na newline");
    eprintf("helvetica, contains\na newline and a trailing newline\n");
    eprintf("forage, contains\nmultiple\nnewlines\n");
    eprintf("edison bulb, contains\nmultiple\nnewlines\aand a trailing newline\n");
}

int main(int argc __attribute__((unused)), char **argv __attribute__((unused)))
{
    simple_test();
    level_test();
    newline_test();
    return 0;
}
