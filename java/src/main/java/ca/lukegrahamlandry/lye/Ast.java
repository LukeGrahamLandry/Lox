package ca.lukegrahamlandry.lye;

import net.bytebuddy.implementation.bytecode.member.MethodReturn;
import net.bytebuddy.implementation.bytecode.member.MethodVariableAccess;

import java.util.List;

public class Ast {
    public abstract static class Stmt {

    }

    public static class BlockStmt extends Stmt {
        public final List<Ast.Stmt> statements;

        public BlockStmt(List<Ast.Stmt> statements) {
            this.statements = statements;
        }

        @Override
        public int hashCode() {
            int h = 3141592;
            for (Ast.Stmt s : statements){
                h += s.hashCode();
            }
            return h;
        }
    }

    public static class DefStmt extends Stmt {
        public final String name;
        public final Expr value;
        public final Type type;
        public final boolean isFinal;
        public final boolean isStatic;

        private DefStmt(String name, Expr value, Type type, boolean isFinal, boolean isStatic) {
            this.name = name;
            this.value = value;
            this.type = type;
            this.isFinal = isFinal;
            this.isStatic = isStatic;
        }

        @Override
        public int hashCode() {
            return this.name.hashCode() * (1 + this.value.hashCode());
        }

        public enum Type {
            VARIABLE,
            FUNCTION,
            CLASS;

            public DefStmt of(String name, boolean isFinal, Expr value){
                return new DefStmt(name, value, this, isFinal, false);
            }

            public DefStmt ofStatic(String name, boolean isFinal, Expr value){
                return new DefStmt(name, value, this, isFinal, true);
            }
        }
    }

    public static class ExprStmt extends Stmt {
        public final Expr value;
        public final Type type;

        private ExprStmt(Expr value, Type type) {
            this.value = value;
            this.type = type;
        }

        @Override
        public int hashCode() {
            return this.value.hashCode() * (1 + this.type.ordinal());
        }

        public enum Type {
            EVALUATE,
            RETURN;

            public ExprStmt of(Expr value){
                return new ExprStmt(value, this);
            }
        }
    }

    public static class IfStmt extends Stmt {
        public final Expr condition;
        public final Stmt thenBlock;
        public final Stmt elseBlock;

        public IfStmt(Expr condition, Stmt thenBlock, Stmt elseBlock) {
            this.condition = condition;
            this.thenBlock = thenBlock;
            this.elseBlock = elseBlock;
        }

        @Override
        public int hashCode() {
            return condition.hashCode() + thenBlock.hashCode() + elseBlock.hashCode();
        }
    }

    public static abstract class Expr {
        public abstract LoxType getType();

        // TODO: Use reflection to generate from java definitions
        public enum LoxType {
            BOOLEAN(MethodReturn.INTEGER, MethodVariableAccess.INTEGER),
            NUMBER(MethodReturn.DOUBLE, MethodVariableAccess.DOUBLE),
            STRING(MethodReturn.REFERENCE, MethodVariableAccess.REFERENCE),
            CALLABLE(MethodReturn.REFERENCE, MethodVariableAccess.REFERENCE),
            NIL(MethodReturn.REFERENCE, MethodVariableAccess.REFERENCE);

            public final MethodReturn asReturn;
            public final MethodVariableAccess asLocalVar;

            LoxType(MethodReturn asReturn, MethodVariableAccess asLocalVar) {
                this.asReturn = asReturn;
                this.asLocalVar = asLocalVar;
            }
        }
    }
    
    public static class BinaryExpr extends Expr {
        public final Expr left;
        public final Expr right;
        public final Op op;

        private BinaryExpr(Expr left, Expr right, Op op) {
            this.left = left;
            this.right = right;
            this.op = op;
        }

        @Override
        public LoxType getType() {
            if (op == Op.AND || op == Op.OR) return LoxType.BOOLEAN;
            if (op == Op.ADD) return left.getType();
            return LoxType.NUMBER;
        }

        @Override
        public int hashCode() {
            return (left.hashCode() + right.hashCode()) * (1 + op.ordinal());
        }

        public enum Op {
            ADD,
            SUBTRACT,
            MULTIPLY,
            DIVIDE,
            POWER,
            AND,
            OR;

            public BinaryExpr of(Expr left, Expr right){
                return new BinaryExpr(left, right, this);
            }
        }
    }

    public static class UnaryExpr extends Expr {
        public final Expr value;
        public final Op op;

        private UnaryExpr(Expr value, Op op) {
            this.value = value;
            this.op = op;
        }

        @Override
        public LoxType getType() {
            return value.getType();
        }

        @Override
        public int hashCode() {
            return value.hashCode() * (1 + op.ordinal());
        }

        public enum Op {
            NONE,
            NEGATE,
            NOT;

            public UnaryExpr of(Expr value){
                return new UnaryExpr(value, this);
            }
        }
    }

    public static class VarExpr extends Expr {
        public final String name;
        public final Expr value;
        public final Type type;
        public final LoxType kind;

        private VarExpr(String name, Expr value, Type type, LoxType kind) {
            this.name = name;
            this.value = value;
            this.type = type;
            this.kind = kind;
        }

        public static VarExpr ofGet(String name, LoxType type){
            return new VarExpr(name, null, Type.GET, type);
        }

        public static VarExpr ofSet(String name, Expr value){
            return new VarExpr(name, value, Type.SET, value.getType());
        }

        @Override
        public int hashCode() {
            return (name.hashCode() + ((value != null ? value.hashCode() : 0))) * (1 + type.ordinal()) * (1 + kind.ordinal());
        }

        @Override
        public LoxType getType() {
            return kind;
        }

        public enum Type {
            GET_THIS,
            GET,
            SET
        }
    }

    public static class LiteralExpr extends Expr {
        public final String strValue;
        public final Double numValue;
        public final Boolean boolVal;

        private LiteralExpr(String strValue, Double numValue, Boolean boolVal) {
            this.strValue = strValue;
            this.numValue = numValue;
            this.boolVal = boolVal;
        }

        public static LiteralExpr of(String str){
            return new LiteralExpr(str, null, null);
        }

        public static LiteralExpr of(double num){
            return new LiteralExpr(null, num, null);
        }

        public static LiteralExpr of(boolean b){
            return new LiteralExpr(null, null, b);
        }

        @Override
        public int hashCode() {
            if (strValue != null) return strValue.hashCode();
            if (numValue != null) return numValue.hashCode();
            if (boolVal != null) return boolVal.hashCode();
            return 9;  // That's the problem with randomness... You can never be sure.
        }

        public static LiteralExpr ofNil(){
            return new LiteralExpr(null, null, null);
        }

        @Override
        public LoxType getType() {
            if (strValue != null) return LoxType.STRING;
            if (numValue != null) return LoxType.NUMBER;
            if (boolVal != null) return LoxType.BOOLEAN;
            return LoxType.NIL;
        }
    }

    public static class ClassExpr extends Expr {
        List<DefStmt> methods;
        List<DefStmt> fields;
        List<DefStmt> staticFields;

        @Override
        public LoxType getType() {
            return null;
        }
    }

    public static class FunctionExpr extends Expr {
        @Override
        public LoxType getType() {
            return null;
        }
    }
}
