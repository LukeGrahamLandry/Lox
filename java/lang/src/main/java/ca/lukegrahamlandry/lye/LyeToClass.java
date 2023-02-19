package ca.lukegrahamlandry.lye;

import ca.lukegrahamlandry.lye.asmstuff.AsmExperiment;
import net.bytebuddy.ByteBuddy;
import net.bytebuddy.dynamic.DynamicType;
import net.bytebuddy.implementation.FixedValue;
import net.bytebuddy.implementation.Implementation;
import org.objectweb.asm.Opcodes;

import static net.bytebuddy.matcher.ElementMatchers.named;

public class LyeToClass implements Opcodes {
    private final String name;
    private final Ast.BlockStmt codeTree;

    public LyeToClass(String name, Ast.BlockStmt codeTree){
        this.name = name;
        this.codeTree = codeTree;
    }

    public static Class<?> compile(String name, Ast.BlockStmt codeTree){
        return new LyeToClass(name, codeTree).compile();
    }

    public Class<?> compile(){
        accept(this.codeTree);

        Implementation impl = new CompileExpression().compile(this.codeTree);

        DynamicType.Unloaded<Object> generated = new ByteBuddy()
                .subclass(Object.class)
                .name("lye.script." + name + ".Main")
                .defineMethod("main", double.class, Opcodes.ACC_PUBLIC | Opcodes.ACC_STATIC)
                    .intercept(impl)
                .method(named("toString"))
                    .intercept(FixedValue.value("<Lye Script: " + name + ">"))
                .make();

        AsmExperiment.writeFile("LyeScriptMain$" + name + ".class", generated.getBytes());

        Class<?> cls = generated
                .load(getClass().getClassLoader())
                .getLoaded();

        return cls;
    }


    public void accept(Ast.BlockStmt stmt){

    }

    public void accept(Ast.Expr expr){
        if (expr instanceof Ast.BinaryExpr){
            acceptBinaryExpr((Ast.BinaryExpr) expr);
        }
    }

    private void acceptBinaryExpr(Ast.BinaryExpr expr) {

    }
}
