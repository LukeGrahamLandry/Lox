package ca.lukegrahamlandry.lye;

import net.bytebuddy.description.method.MethodDescription;
import net.bytebuddy.implementation.Implementation;
import net.bytebuddy.implementation.bytecode.ByteCodeAppender;
import net.bytebuddy.implementation.bytecode.StackManipulation;
import net.bytebuddy.jar.asm.Label;
import net.bytebuddy.jar.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;

import java.util.ArrayList;
import java.util.List;

public class CompileBlock implements Opcodes {
    CompileExpression scope;

    public void compile(Ast.Stmt stmt){
        if (stmt instanceof Ast.BlockStmt){
            CompileExpression outerScope = scope;
            scope = new CompileExpression(outerScope);
            for (Ast.Stmt s : ((Ast.BlockStmt) stmt).statements){
                compile(s);
            }
            scope = outerScope;
        }
        else if (stmt instanceof Ast.ExprStmt || stmt instanceof Ast.DefStmt){
            // forward to CompileExpression
        }
        else if (stmt instanceof Ast.IfStmt){
            Ast.IfStmt s = (Ast.IfStmt) stmt;
            ByteCodeAppender bytes = new ByteCodeAppender() {
                @Override
                public Size apply(MethodVisitor methodVisitor, Implementation.Context implementationContext, MethodDescription instrumentedMethod) {
                    // todo: actually emit this
                    List<StackManipulation> condition = new ArrayList<>();
                    scope.compile(s.condition, condition);
                    scope.compile(Ast.LiteralExpr.of(false), condition);

                    // todo actually put the label somewhere
                    methodVisitor.visitJumpInsn(IF_ICMPEQ, new Label());

                    return null;
                }
            };
        }
    }
}
