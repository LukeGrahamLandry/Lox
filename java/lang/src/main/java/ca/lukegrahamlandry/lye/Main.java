package ca.lukegrahamlandry.lye;


import java.lang.reflect.Method;
import java.util.Arrays;

public class Main {
    public static void main(String[] args) {
        Ast.BlockStmt code = new Ast.BlockStmt(Arrays.asList(
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

        Class<?> cls = LyeToClass.compile("example", code);
        runMain(cls);
        String loxCode = "var x = 10 * 10 - 50; return x;";
        runMain(LyeToClass.compile("fromlox", LoxToLyeTree.compile(loxCode)));

    }

    public static void runMain(Class<?> cls){
        System.out.println("----------------------------");
        for (Method m : cls.getMethods()){
            System.out.println("- " + m.getName() + " returns " + m.getReturnType() + " takes " + m.getParameterCount() + " args.");
        }
        try {
            System.out.println("cls name: " + cls.getName());
            System.out.println("inst as string: " + cls.newInstance());
            System.out.println("methods:");
            Method main = cls.getDeclaredMethod("main");
            double result = (double) main.invoke(null);
            System.out.println("result: " + result);
        } catch (Exception e) {
            throw new RuntimeException(e);
        }
    }

}
