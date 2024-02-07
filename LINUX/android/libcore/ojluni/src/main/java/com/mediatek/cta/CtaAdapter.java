package com.mediatek.cta;

import dalvik.system.PathClassLoader;

import java.lang.ClassLoader;
import java.lang.reflect.Method;

/**
 * This class implements as a CTA adapter
 * @hide
 */
public final class CtaAdapter {

    enum Value {
      NULL_VALUE,
      CTA_ENABLE,
      CTA_DISABLE
    };

    private static Value flag = Value.NULL_VALUE;
    private static Method ctaPermissionChecker = null;
    private static String jarPath = "system/framework/mediatek-cta.jar";
    private static String className = "com.mediatek.cta.CtaUtils";
    private static String methodName = "enforceCheckPermission";
    private static String emailPermission = "com.mediatek.permission.CTA_SEND_EMAIL";
    private static String emailMessage = "Send emails";

    /** @hide */
    public static boolean isSendingPermitted(int port) {
        System.out.println("[com.mediatek.cta.CtaAdapter]:check permission begin!");
        try {
            if (ctaPermissionChecker == null && flag == Value.NULL_VALUE) {
                ClassLoader classLoader = new PathClassLoader(jarPath,
                        CtaAdapter.class.getClassLoader());
                Class<?> cls = Class.forName(className, false, classLoader);
                ctaPermissionChecker = cls.getDeclaredMethod(methodName,
                        String.class, String.class);
                ctaPermissionChecker.setAccessible(true);
                flag = Value.CTA_ENABLE;
            }

            if (port == 25 || port == 465 || port == 587) {
                return (Boolean) ctaPermissionChecker.invoke(null, emailPermission, emailMessage);
            }

        } catch (ReflectiveOperationException e) {
            flag = Value.CTA_DISABLE;
            System.out.println("[com.mediatek.cta.CtaAdapter] e:" + e);
            if (e.getCause() instanceof SecurityException) {
                throw new SecurityException(e.getCause());
            } else if (e.getCause() instanceof ClassNotFoundException) {
                // for project not include CTA feature
            }
        } catch (Throwable ee) {
            if (ee instanceof NoClassDefFoundError) {
                flag = Value.CTA_DISABLE;
                System.out.println("[com.mediatek.cta.CtaAdapter] ee:" + ee);
            }
        }
        return true;
    }
}
