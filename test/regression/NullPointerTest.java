/*
 * test that caught null pointers exceptions in finalizers work correctly
 * and that local variables are accessible in null pointer exception handlers.
 */
import java.io.*;

public class NullPointerTest {

    static String s;

    public static void main(String[] args) {
	  System.out.println(tryfinally() + s);
	  try {
		// Note: String.concat("") will not dereference 'this'
		((String)null).concat("");
	  	System.out.println("FAILED!");
	  } catch (NullPointerException e) {
	  	System.out.println("This is good too");
	  }
    }

    public static String tryfinally() {
	String yuck = null;
	String local_s = null;

	try {
	    return "This is ";
	} finally {
	    try {
		local_s = "Perfect";
		/* trigger null pointer exception */
		String x = yuck.toLowerCase();
	    } catch (Exception _) {
		/* 
		 * when the null pointer exception is caught, we must still
		 * be able to access local_s.
		 * Our return address for the finally clause must also still
		 * be intact.
		 */
		s = local_s;
	    }
	}
    }
}


/* Expected Output:
This is Perfect
This is good too
*/
