package ca.lukegrahamlandry.lye.asmstuff;

import org.objectweb.asm.*;

import static org.objectweb.asm.ClassWriter.COMPUTE_FRAMES;

public class WrappedWriter implements Opcodes {
    // Should be a stack of them, so I can happily recurse down and add new ones as the script defines classes.
    // Think it makes sense to just combine them all into nested classes so one script becomes one big java class with a main method.
    public ClassWriter classWriter;
    public FieldVisitor fieldVisitor;
    public RecordComponentVisitor recordComponentVisitor;
    public MethodVisitor methodVisitor;
    public AnnotationVisitor annotationVisitor;

    public WrappedWriter(String name){
        classWriter = new ClassWriter(COMPUTE_FRAMES);

        classWriter.visit(V1_8, ACC_PUBLIC | ACC_SUPER, "lye/script/" + name + "/Main", null, "java/lang/Object", null);
        classWriter.visitSource("Main.java", null);
    }

    public byte[] end(){
        classWriter.visitEnd();
        return new byte[0];
    }
}
