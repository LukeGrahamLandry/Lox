package ca.lukegrahamlandry.lye.asmstuff;

import ca.lukegrahamlandry.lye.LoxToLyeTree;
import com.craftinginterpreters.lox.*;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.util.ASMifier;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;
import java.util.List;

public class AsmExperiment {
    public static void testLox(){
        String code = "fun add(a, b){return a+b;} fun test(){print 1 + 2;return add(3, 4);}";
        Scanner scanner = new Scanner(code);
        List<Token> tokens = scanner.scanTokens();
        Parser parser = new Parser(tokens);
        List<Stmt> statements = parser.parse();

        AstPrinter printer = new AstPrinter();
        for (Stmt s : statements){
            System.out.println(printer.print(s));
        }

//        ClassWriter results = asm.accept("test", statements);
//        byte[] data = results.toByteArray();
//        writeFile("Main.class", data);
    }

    public static void writeFile(String filename, byte[] data){
        File file = new File("build", filename);
        try (FileOutputStream out = new FileOutputStream(file)) {
            out.write(data);
        } catch (IOException e) {
            throw new RuntimeException(e);
        }
    }

    public static void asmify() throws IOException {
        String[] arguments = new String[] {"ca.lukegrahamlandry.lye.Main"};
        ASMifier.main(arguments);
    }

    public static void testWrite() throws IOException {
        ClassWriter results = new ClassWriter(0);
        new ClassReader("ca.lukegrahamlandry.lye.Main").accept(results, 0);
        byte[] data = results.toByteArray();
        writeFile("LyeMainReadWrite.class", data);

        results = new ClassWriter(0);
        new ClassReader("ca.lukegrahamlandry.lye.Main$StuffDoer").accept(results, 0);
        data = results.toByteArray();
        writeFile("LyeMainReadWriteInner.class", data);

        runMain(loadClass("ca.lukegrahamlandry.lye.Main$StuffDoer", data));
    }

    public static void runMain(Class<?> clazz){
        System.out.println("Doing reflection:");
        try {
            Method main = clazz.getDeclaredMethod("main", String[].class);
            main.invoke(null, new Object[]{new String[0]});
        } catch (NoSuchMethodException | InvocationTargetException | IllegalAccessException e) {
            throw new RuntimeException(e);
        }
    }

    public static Class loadClass(String className, byte[] b) {
        // Override defineClass (as it is protected) and define the class.
        Class clazz = null;
        try {
            ClassLoader loader = ClassLoader.getSystemClassLoader();
            Class cls = Class.forName("java.lang.ClassLoader");
            java.lang.reflect.Method method =
                    cls.getDeclaredMethod(
                            "defineClass",
                            String.class, byte[].class, int.class, int.class);

            // Protected method invocation.
            method.setAccessible(true);
            try {
                Object[] args =
                        new Object[] { className, b, new Integer(0), new Integer(b.length)};
                clazz = (Class) method.invoke(loader, args);
            } finally {
                method.setAccessible(false);
            }
        } catch (Exception e) {
            e.printStackTrace();
            System.exit(1);
        }
        return clazz;
    }

    public static class StuffDoer {
        public static void main(String[] args) {
            new StuffDoer().doStuff();
        }
        public void doStuff(){
            System.out.println("Hello world!");

            int a = (5 + 5) - 1;
            int b = 1 + 2 * a;
            System.out.println(b);
        }
    }

    public void math(){
        int a = (5 + 5) - 1;
        int b = 1 + 2 * a;
        int c = 100 / b;
        System.out.println(c);
    }
}
