package ca.lukegrahamlandry.lye;

public class DecompileTree {
    public static void print(Ast.Stmt stmt){
        new DecompileTree().visit(stmt);
    }

    int depth = 0;
    private void out(String s){
        StringBuilder ss = new StringBuilder();
        for (int i=0;i<depth;i++){
            ss.append("  ");
        }
        ss.append(s);
        System.out.println(ss);
    }

    public void visit(Ast.Stmt stmt){
        if (stmt instanceof Ast.BlockStmt){
            out("{");
            depth++;
            for (Ast.Stmt s : ((Ast.BlockStmt) stmt).statements){
                visit(s);
            }
            depth--;
            out("}");
        } else if (stmt instanceof Ast.DefStmt){
            switch (((Ast.DefStmt) stmt).type){
                case VARIABLE:
                    out("var " + ((Ast.DefStmt) stmt).name + " = " + visit(((Ast.DefStmt) stmt).value) + ";");
                    break;
                case FUNCTION:
                    break;
                case CLASS:
                    break;
            }
        } else if (stmt instanceof Ast.ExprStmt) {
            switch (((Ast.ExprStmt) stmt).type){
                case EVALUATE:
                    out(visit(((Ast.ExprStmt) stmt).value) + ";");
                    break;
                case RETURN:
                    out("return " + visit(((Ast.ExprStmt) stmt).value) + ";");
                    break;
            }
        }
    }

    public String visit(Ast.Expr expr){
        String result = "";
        if (expr instanceof Ast.BinaryExpr){
            result = "(";
            result += visit(((Ast.BinaryExpr) expr).left);
            switch (((Ast.BinaryExpr) expr).op){
                case ADD:
                    result += " + ";
                    break;
                case SUBTRACT:
                    result += " - ";
                    break;
                case MULTIPLY:
                    result += " * ";
                    break;
                case DIVIDE:
                    result += " / ";
                    break;
                case POWER:
                    result += " ** ";
                    break;
                case AND:
                    result += " and ";
                    break;
                case OR:
                    result += " or ";
                    break;
            }
            result += visit(((Ast.BinaryExpr) expr).right);
            result += ")";
        } else if (expr instanceof Ast.LiteralExpr){
            switch (expr.getType()){
                case BOOLEAN:
                    result = ((Ast.LiteralExpr) expr).boolVal.toString();
                    break;
                case NUMBER:
                    result = ((Ast.LiteralExpr) expr).numValue.toString();
                    break;
                case STRING:
                    result = ((Ast.LiteralExpr) expr).strValue;
                    break;
                case CALLABLE:
                    break;
                case NIL:
                    result = "nil";
                    break;
            }
        } else if (expr instanceof Ast.VarExpr) {
            switch (((Ast.VarExpr) expr).type){
                case GET_THIS:
                    result = "this";
                    break;
                case GET:
                    result = ((Ast.VarExpr) expr).name;
                    break;
                case SET:
                    result = "(";
                    result += ((Ast.VarExpr) expr).name;
                    result += visit(((Ast.VarExpr) expr).value);
                    result += ")";
                    break;
            }
        }

        return result;
    }
}
