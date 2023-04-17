package ca.lukegrahamlandry.lye;


import java.lang.reflect.Method;
import java.util.Arrays;

public class Main {
    public static void main(String[] args) {
        Ast.BlockStmt code1 = new Ast.BlockStmt(Arrays.asList(
                Ast.DefStmt.Type.VARIABLE.of("tempvar", false,
                        Ast.BinaryExpr.Op.DIVIDE.of(
                                Ast.BinaryExpr.Op.ADD.of(
                                        Ast.BinaryExpr.Op.MULTIPLY.of(
                                                Ast.LiteralExpr.of(10),
                                                Ast.BinaryExpr.Op.SUBTRACT.of(
                                                        Ast.LiteralExpr.of(20),
                                                        Ast.LiteralExpr.of(10)
                                                )
                                        ),
                                        Ast.BinaryExpr.Op.ADD.of(
                                                Ast.LiteralExpr.of(20),
                                                Ast.LiteralExpr.of(30)
                                        )
                                ),
                                Ast.LiteralExpr.of(2)
                        )
                ),
                Ast.ExprStmt.Type.RETURN.of(
                        Ast.VarExpr.ofGet("tempvar", Ast.Expr.LoxType.NUMBER)
                )
        ));

        String loxCode = "var tempvar = (10.0 * (20.0 - 10.0) + 20.0 + 30.0) / 2.0; return tempvar;";
        Ast.BlockStmt code2 = LoxToLyeTree.compile(loxCode);

        System.out.println("Success: " + (code1.hashCode() == code2.hashCode()));
        runMain(LyeToClass.compile("from_api", code1));
        runMain(LyeToClass.compile("from_lox", code2));

        TypeAst.whoIsTheFairestOfThemAll(String.class);
        TypeAst.memo.forEach(((s, lyeType) -> System.out.println(lyeType.toString())));
    }

    public static void runMain(Class<?> cls){
        for (Method m : cls.getMethods()){
            System.out.println("- " + m.getName() + " returns " + m.getReturnType() + " takes " + m.getParameterCount() + " args.");
        }
        try {
            System.out.println("cls name: " + cls.getName());
            System.out.println("inst as string: " + cls.newInstance());
            Method main = cls.getDeclaredMethod("main");
            double result = (double) main.invoke(null);
            System.out.println("result: " + result);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }
}
