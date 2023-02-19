package ca.lukegrahamlandry.lye;

import net.bytebuddy.dynamic.scaffold.InstrumentedType;
import net.bytebuddy.implementation.Implementation;
import net.bytebuddy.implementation.bytecode.*;
import net.bytebuddy.implementation.bytecode.constant.DoubleConstant;
import net.bytebuddy.implementation.bytecode.constant.IntegerConstant;
import net.bytebuddy.jar.asm.Label;
import net.bytebuddy.jar.asm.Type;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

public class CompileExpression {
    public List<StackManipulation> compile(List<Ast.Stmt> code){
        List<StackManipulation> actions = new ArrayList<>();
        for (Ast.Stmt statement : code){
            if (statement instanceof Ast.DefStmt){
                Ast.DefStmt e = (Ast.DefStmt) statement;
                if (e.type == Ast.DefStmt.Type.VARIABLE){
                    if (variables.containsKey(e.name)){
                        throw new RuntimeException("Redeclare variable named " + e.name);
                    }

                    variables.put(e.name, variables.size() * 2);  // doubles take two slots. non-static will need to offset + 1 for 'this' i think
                    compile(e.value, actions);
                    actions.add(e.value.getType().asLocalVar.storeAt(variables.get(e.name)));
                }
            }
            else if (statement instanceof Ast.ExprStmt) {
                Ast.ExprStmt stmt = (Ast.ExprStmt) statement;

                compile(stmt.value, actions);
                if (stmt.type == Ast.ExprStmt.Type.RETURN){
                    actions.add(stmt.value.getType().asReturn);
                    break;
                }
            }
        }
        return actions;
    }

    private HashMap<String, Integer> variables = new HashMap<>();

    public Implementation compile(Ast.BlockStmt codeTree) {
        List<StackManipulation> actions = compile(codeTree.statements);
        return new CompileExpression.MathImpl(actions, variables.size() * 2);  // doubles take two slots
    }

    public void compile(Ast.Expr expr, List<StackManipulation> actions){
        if (expr instanceof Ast.BinaryExpr){
            Ast.BinaryExpr e = (Ast.BinaryExpr) expr;
            compile(e.left, actions);
            compile(e.right, actions);
            switch (e.op){
                case ADD:
                    if (e.getType() == Ast.Expr.LoxType.NUMBER){
                        actions.add(Addition.DOUBLE);
                    } else {
                        // call string.concat
                    }

                    break;
                case SUBTRACT:
                    actions.add(Subtraction.DOUBLE);
                    break;
                case MULTIPLY:
                    actions.add(Multiplication.DOUBLE);
                    break;
                case DIVIDE:
                    actions.add(Division.DOUBLE);
                    break;
                case POWER:
                    break;
                case AND:
                    break;
                case OR:
                    break;
            }
        } else if (expr instanceof Ast.LiteralExpr) {
            Ast.LiteralExpr e = (Ast.LiteralExpr) expr;
            switch (e.getType()){
                case BOOLEAN:
                    actions.add(IntegerConstant.forValue(e.boolVal ? 1 : 0));
                    break;
                case NUMBER:
                    actions.add(DoubleConstant.forValue(e.numValue));
                    break;
                case STRING:
                    break;
                case CALLABLE:
                    break;
                case NIL:
                    break;
            }
        } else if (expr instanceof Ast.VarExpr){
            Ast.VarExpr e = (Ast.VarExpr) expr;
            switch (e.type){
                case GET_THIS:
                    break;
                case GET:
                    if (!variables.containsKey(e.name)){
                        throw new RuntimeException("Undefined variable named " + e.name);
                    }

                    actions.add(e.getType().asLocalVar.loadFrom(variables.get(e.name)));
                    break;
                case SET:
                    if (!variables.containsKey(e.name)){
                        throw new RuntimeException("Undefined variable named " + e.name);
                    }

                    compile(e.value, actions);
                    actions.add(e.getType().asLocalVar.storeAt(variables.get(e.name)));
                    break;
            }
        }
    }

    public class MathImpl implements Implementation {
        private final List<StackManipulation> actions;
        private final int localVariableCount;

        public MathImpl(List<StackManipulation> actions, int localVariableCount){
            this.actions = actions;
            this.localVariableCount = localVariableCount;
        }

        @Override
        public InstrumentedType prepare(InstrumentedType instrumentedType) {
            return instrumentedType;
        }

        @Override
        public ByteCodeAppender appender(Target implementationTarget) {
            return (methodVisitor, implementationContext, instrumentedMethod) -> {
                Label blockStart = new Label();
                Label blockEnd = new Label();

                methodVisitor.visitLabel(blockStart);
                StackManipulation.Compound actions = new StackManipulation.Compound(MathImpl.this.actions);
                StackManipulation.Size size = actions.apply(methodVisitor, implementationContext);
                methodVisitor.visitLabel(blockEnd);
                CompileExpression.this.variables.forEach((name, offset) -> {
                    methodVisitor.visitLocalVariable(name, Type.getDescriptor(double.class), null, blockStart, blockEnd, offset);
                });

                return new ByteCodeAppender.Size(size.getMaximalSize(), instrumentedMethod.getStackSize() + localVariableCount);
            };
        }
    }
}
