package ca.lukegrahamlandry.lye;

import com.craftinginterpreters.lox.*;

import java.util.ArrayList;
import java.util.List;

public class LoxToLyeTree implements Expr.Visitor<Ast.Expr>, Stmt.Visitor<Ast.Stmt> {
    public static Ast.BlockStmt compile(String loxCode){
        Scanner scanner = new Scanner(loxCode);
        List<Token> tokens = scanner.scanTokens();
        Parser parser = new Parser(tokens);
        List<Stmt> statements = parser.parse();
        return new LoxToLyeTree().visit(statements);
    }

    public Ast.BlockStmt visit(List<Stmt> statements){
        List<Ast.Stmt> tree = new ArrayList<>();
        for (Stmt stmt : statements){
            tree.add(stmt.accept(this));
        }
        return new Ast.BlockStmt(tree);
    }

    public Ast.Expr visit(Expr expr){
        return expr.accept(this);
    }

    public Ast.Stmt visit(Stmt stmt){
        return stmt.accept(this);
    }

    @Override
    public Ast.Expr visitAssignExpr(Expr.Assign expr) {
        return Ast.VarExpr.ofSet(expr.name.lexeme, visit(expr.value));
    }

    @Override
    public Ast.Expr visitBinaryExpr(Expr.Binary expr) {
        switch (expr.operator.type){
            case PLUS:
                return Ast.BinaryExpr.Op.ADD.of(visit(expr.left), visit(expr.right));
            case MINUS:
                return Ast.BinaryExpr.Op.SUBTRACT.of(visit(expr.left), visit(expr.right));
            case STAR:
                return Ast.BinaryExpr.Op.MULTIPLY.of(visit(expr.left), visit(expr.right));
            case SLASH:
                return Ast.BinaryExpr.Op.DIVIDE.of(visit(expr.left), visit(expr.right));
        }
        return null;
    }

    @Override
    public Ast.Expr visitCallExpr(Expr.Call expr) {
        return null;
    }

    @Override
    public Ast.Expr visitGetExpr(Expr.Get expr) {  // of class field
        return null;
    }

    @Override
    public Ast.Expr visitGroupingExpr(Expr.Grouping expr) {
        return null;
    }

    @Override
    public Ast.Expr visitLiteralExpr(Expr.Literal expr) {
        if (expr.value instanceof String){
            return Ast.LiteralExpr.of((String) expr.value);
        }
        if (expr.value instanceof Double){
            return Ast.LiteralExpr.of((Double) expr.value);
        }
        if (expr.value instanceof Boolean){
            return Ast.LiteralExpr.of((Boolean) expr.value);
        }
        return Ast.LiteralExpr.ofNil();
    }

    @Override
    public Ast.Expr visitLogicalExpr(Expr.Logical expr) {
        return null;
    }

    @Override
    public Ast.Expr visitSetExpr(Expr.Set expr) {  // of class field
        return null;
    }

    @Override
    public Ast.Expr visitSuperExpr(Expr.Super expr) {
        return null;
    }

    @Override
    public Ast.Expr visitThisExpr(Expr.This expr) {
        return null;
    }

    @Override
    public Ast.Expr visitUnaryExpr(Expr.Unary expr) {
        return null;
    }

    @Override
    public Ast.Expr visitVariableExpr(Expr.Variable expr) {
        return Ast.VarExpr.ofGet(expr.name.lexeme, Ast.Expr.LoxType.NUMBER);  // TODO: types
    }

    @Override
    public Ast.Stmt visitBlockStmt(Stmt.Block stmt) {
        return visit(stmt.statements);
    }

    @Override
    public Ast.Stmt visitClassStmt(Stmt.Class stmt) {
        return null;
    }

    @Override
    public Ast.Stmt visitExpressionStmt(Stmt.Expression stmt) {
        return Ast.ExprStmt.Type.EVALUATE.of(visit(stmt.expression));
    }

    @Override
    public Ast.Stmt visitFunctionStmt(Stmt.Function stmt) {
        return null;
    }

    @Override
    public Ast.Stmt visitIfStmt(Stmt.If stmt) {
        return null;
    }

    @Override
    public Ast.Stmt visitPrintStmt(Stmt.Print stmt) {
        return null;
    }

    @Override
    public Ast.Stmt visitReturnStmt(Stmt.Return stmt) {
        return Ast.ExprStmt.Type.RETURN.of(visit(stmt.value));
    }

    @Override
    public Ast.Stmt visitVarStmt(Stmt.Var stmt) {
        return Ast.DefStmt.Type.VARIABLE.of(stmt.name.lexeme, false, stmt.initializer == null ? Ast.LiteralExpr.ofNil() : visit(stmt.initializer));
    }

    @Override
    public Ast.Stmt visitWhileStmt(Stmt.While stmt) {
        return null;
    }
}
