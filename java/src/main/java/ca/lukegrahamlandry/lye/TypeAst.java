package ca.lukegrahamlandry.lye;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

public class TypeAst {
    public static class LyeType {

    }

    public static class Callable extends LyeType {
        public List<LyeType> arguments = new ArrayList<>();
        public List<LyeType> returns = new ArrayList<>();

        @Override
        public String toString() {
            StringBuilder s = new StringBuilder("(");
            for (LyeType arg : arguments){
                s.append(arg.toString()).append(", ");
            }
            if (s.length() > 2){
                s.deleteCharAt(s.length() - 1);
                s.deleteCharAt(s.length() - 1);
            }
            return s + ") -> " + returns.get(0).toString();
        }
    }

    public static class Klass extends LyeType {
        public String name;
        public Klass superClass;
        public Map<String, Callable> methods = new HashMap<>();
        public Map<String, LyeType> fields = new HashMap<>();
        public Map<String, LyeType> staticFields = new HashMap<>();

        public String toString(){
            StringBuilder s = new StringBuilder("Class: " + name + " \n");
            s.append("- Methods: \n");
            for (Map.Entry<String, Callable> m : methods.entrySet()){
                s.append("    - ").append(m.getKey()).append(": ").append(m.getValue().toString()).append("\n");
            }
            s.append("- Fields: \n");
            for (Map.Entry<String, LyeType> m : fields.entrySet()){
                s.append("  - ").append(m.getKey()).append(": ").append(m.getValue().toString()).append("\n");
            }
            s.append("- Static Fields: \n");
            for (Map.Entry<String, LyeType> m : staticFields.entrySet()){
                s.append("    - ").append(m.getKey()).append(": ").append(m.getValue().toString()).append("\n");
            }
            return s.toString();
        }
    }

    public static class KlassRef extends LyeType {
        private String name;
        private KlassRef(String name){
            this.name = name;
        }

        public LyeType get(){
            return memo.get(name);
        }

        @Override
        public String toString() {
            return "Class<" + name + ">";
        }

        public static KlassRef of(String name){
            return (KlassRef) pool.getOrDefault(name, new KlassRef(name));
        }
    }

    public static class Array extends LyeType {
        public LyeType content;

        public Array(LyeType content) {
            this.content = content;
        }

        @Override
        public String toString() {
            return "Array<" + content.toString() + ">";
        }
    }

    public static class Primitive extends LyeType {

    }

    public static LyeType mirrorMirrorOnTheWall(String cls){
        try {
            return whoIsTheFairestOfThemAll(Class.forName(cls));
        } catch (ClassNotFoundException e) {
            throw new RuntimeException(e);
        }
    }

    public static Map<String, LyeType> memo = new HashMap<>();
    public static Map<String, LyeType> pool = new HashMap<>();
    public static LyeType whoIsTheFairestOfThemAll(Class<?> cls){
        if (cls.isArray()){
            return new Array(whoIsTheFairestOfThemAll(cls.getComponentType()));
        }

        if (!memo.containsKey(cls.getName())) {
            memo.put(cls.getName(), null);

            Klass klass = new Klass();
            klass.name = cls.getName();

            for (Method method : cls.getMethods()){
                Callable callable = new Callable();
                for (Class<?> arg : method.getParameterTypes()){
                    callable.arguments.add(whoIsTheFairestOfThemAll(arg));
                }
                callable.returns.add(whoIsTheFairestOfThemAll(method.getReturnType()));
                klass.methods.put(method.getName(), callable);
            }

            memo.put(cls.getName(), klass);
        }

        return KlassRef.of(cls.getName());
    }
}
