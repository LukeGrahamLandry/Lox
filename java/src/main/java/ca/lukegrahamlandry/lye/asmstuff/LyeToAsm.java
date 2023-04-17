package ca.lukegrahamlandry.lye.asmstuff;

import ca.lukegrahamlandry.lye.Ast;
import org.objectweb.asm.Opcodes;

public class LyeToAsm implements Opcodes {
    WrappedWriter out;
    public static byte[] compile(String name, Ast.BlockStmt codeTree){
        WrappedWriter out = new WrappedWriter(name);
        new LyeToAsm(out).accept(codeTree);
        return out.end();
    }

    public LyeToAsm(WrappedWriter out){
        this.out = out;
    }

    public void accept(Ast.BlockStmt stmt){

    }

    public void accept(Ast.Expr expr){

    }
}
