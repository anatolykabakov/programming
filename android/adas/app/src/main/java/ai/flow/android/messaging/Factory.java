package messaging;

import android.content.Context;
import org.zeromq.ZMQ;

public class Factory {
    private static ZMQ.Context context;
    private static PortMap portmap;
    
    public static ZMQ.Context getContext(){
        if (context == null) {
            context = ZMQ.context(4);
        }
        return context;
    }
    
    public static PortMap getPortmap(){
        return getPortmap(null);
    }
    
    public static PortMap getPortmap(Context androidContext){
        if (portmap == null) {
            portmap = new PortMap().load(androidContext);
        }
        return portmap;
    }
    
    public static ZMQ.Context getNewContext(){
        return ZMQ.context(4);
    }
    
    public static void initialize(Context androidContext) {
        getContext();
        getPortmap(androidContext);
    }
    
    public static void cleanup() {
        if (context != null) {
            context.close();
            context = null;
        }
        portmap = null;
    }
}
    