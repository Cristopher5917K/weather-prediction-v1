#include <stdio.h>

#define ZONAS 5
#define DIAS 30
#define FUT 7

/* Imprime simultáneamente en consola y en el archivo de salida */
#define print_both(...)              \
    do {                             \
        fprintf(out, __VA_ARGS__);   \
        printf(__VA_ARGS__);         \
    } while (0)

/* Límites de referencia (ug/m3) */
const float LIM_CO   = 4000.0f;
const float LIM_NO2  = 25.0f;
const float LIM_PM25 = 15.0f;
const float LIM_SO2  = 40.0f;

const char* nombres_zonas[ZONAS] = {
    "Belisario", "Carapungo", "Centro", "Cotocollao", "El Camal"
};

typedef struct {
    const char* nombre;
    float hist_CO[DIAS], hist_NO2[DIAS], hist_PM25[DIAS], hist_SO2[DIAS];
    float pred_CO[FUT], pred_NO2[FUT], pred_PM25[FUT], pred_SO2[FUT];
    float act_CO, act_NO2, act_PM25, act_SO2;
} Zona;

float promedio(const float *v, int n) {
    float s = 0;
    for (int i = 0; i < n; ++i) s += v[i];
    return s / n;
}

const char* clasificar(float val, float lim) {
    if (val < 0.5f * lim) return "Bajo";
    if (val <= lim) return "Normal";
    return "Muy elevado";
}

void leer_csv(const char* nombre_archivo, Zona z[], float campo[ZONAS][DIAS], int tipo) {
    FILE* f = fopen(nombre_archivo, "r");
    if (!f) { printf("Error abriendo %s\n", nombre_archivo); return; }
    char linea[512];
    fgets(linea, sizeof(linea), f);
    for (int i = 0; i < DIAS; ++i) {
        fgets(linea, sizeof(linea), f);
        sscanf(linea, "%*d,%f,%f,%f,%f,%f",
               &campo[0][i], &campo[1][i], &campo[2][i], &campo[3][i], &campo[4][i]);
    }
    fclose(f);

    for (int j = 0; j < ZONAS; ++j)
        for (int i = 0; i < DIAS; ++i) {
            if (tipo == 1) z[j].hist_CO[i] = campo[j][i];
            else if (tipo == 2) z[j].hist_NO2[i] = campo[j][i];
            else if (tipo == 3) z[j].hist_PM25[i] = campo[j][i];
            else if (tipo == 4) z[j].hist_SO2[i] = campo[j][i];
        }
}

void leer_pred(const char* nombre_archivo, Zona z[], float campo[ZONAS][FUT], int tipo) {
    FILE* f = fopen(nombre_archivo, "r");
    if (!f) { printf("Error abriendo %s\n", nombre_archivo); return; }
    char linea[512];
    fgets(linea, sizeof(linea), f);
    for (int i = 0; i < FUT; ++i) {
        fgets(linea, sizeof(linea), f);
        sscanf(linea, "%*d,%f,%f,%f,%f,%f",
               &campo[0][i], &campo[1][i], &campo[2][i], &campo[3][i], &campo[4][i]);
    }
    fclose(f);

    for (int j = 0; j < ZONAS; ++j)
        for (int i = 0; i < FUT; ++i) {
            if (tipo == 1) z[j].pred_CO[i] = campo[j][i];
            else if (tipo == 2) z[j].pred_NO2[i] = campo[j][i];
            else if (tipo == 3) z[j].pred_PM25[i] = campo[j][i];
            else if (tipo == 4) z[j].pred_SO2[i] = campo[j][i];
        }
}

void pedir_lecturas(Zona z[]) {
    float valor;
    int res;
    printf("\n=== Ingreso de niveles actuales por zona (valores en ug/m3) ===\n");
    for (int i = 0; i < ZONAS; ++i) {
        printf("\nZona: %s\n", z[i].nombre);

        // CO
        do {
            printf("CO   (max %.0f): ", LIM_CO);
            res = scanf("%f", &valor);
            while(getchar() != '\n'); // Limpiar buffer
            if (res != 1 || valor < 0)
                printf("Valor invalido. Ingrese un numero positivo.\n");
        } while (res != 1 || valor < 0);
        z[i].act_CO = valor;

        // NO2
        do {
            printf("NO2  (max %.0f): ", LIM_NO2);
            res = scanf("%f", &valor);
            while(getchar() != '\n');
            if (res != 1 || valor < 0)
                printf("Valor invalido. Ingrese un numero positivo.\n");
        } while (res != 1 || valor < 0);
        z[i].act_NO2 = valor;

        // PM2.5
        do {
            printf("PM2.5(max %.1f): ", LIM_PM25);
            res = scanf("%f", &valor);
            while(getchar() != '\n');
            if (res != 1 || valor < 0)
                printf("Valor invalido. Ingrese un numero positivo.\n");
        } while (res != 1 || valor < 0);
        z[i].act_PM25 = valor;

        // SO2
        do {
            printf("SO2  (max %.0f): ", LIM_SO2);
            res = scanf("%f", &valor);
            while(getchar() != '\n');
            if (res != 1 || valor < 0)
                printf("Valor invalido. Ingrese un numero positivo.\n");
        } while (res != 1 || valor < 0);
        z[i].act_SO2 = valor;
    }
}

void imprimir_pred_lineal(FILE *ft, FILE *fv, FILE *fh, FILE *out, Zona z[]) {
    float temp[FUT], viento[FUT], hum[FUT];
    char dummy[512];

    fgets(dummy, sizeof dummy, ft);
    fgets(dummy, sizeof dummy, fv);
    fgets(dummy, sizeof dummy, fh);

    for (int d = 0; d < FUT; d++) {
        fscanf(ft, "%f,", &temp[d]);
        fscanf(fv, "%f,", &viento[d]);
        fscanf(fh, "%f,", &hum[d]);
    }

    const char* nombre_contam[4] = {"CO", "SO2", "NO2", "PM2.5"};
    print_both("\n=== Predicción con regresión lineal múltiple (Temperatura, Viento, Humedad) ===\n");

    for (int c = 0; c < 4; ++c) {
        print_both("\n--- Contaminante: %s ---\n", nombre_contam[c]);
        print_both(" Dia |", "");
        for (int zt = 0; zt < ZONAS; ++zt) print_both(" %-11s|", z[zt].nombre);
        print_both("\n-----|");
        for (int i = 0; i < ZONAS; ++i) print_both("------------|");
        print_both("\n");

        for (int d = 0; d < FUT; ++d) {
            print_both(" %3d |", d + 1);
            for (int zt = 0; zt < ZONAS; ++zt) {
                float pred;
                if (c == 0) pred = 10 + 0.5f * temp[d] - 0.3f * viento[d] + 0.2f * hum[d];
                else if (c == 1) pred = 2 + 0.3f * temp[d] - 0.25f * viento[d] + 0.15f * hum[d];
                else if (c == 2) pred = 5 + 0.4f * temp[d] - 0.2f * viento[d] + 0.3f * hum[d];
                else pred = 3 + 0.6f * temp[d] - 0.1f * viento[d] + 0.25f * hum[d];
                print_both(" %10.2f |", pred);
            }
            print_both("\n");
        }
    }
}

void procesar(Zona* z, FILE* out) {
    const char* cCO = clasificar(z->act_CO, LIM_CO);
    const char* cNO2 = clasificar(z->act_NO2, LIM_NO2);
    const char* cPM = clasificar(z->act_PM25, LIM_PM25);
    const char* cSO2 = clasificar(z->act_SO2, LIM_SO2);

    float mCO = promedio(z->hist_CO, DIAS);
    float mNO2 = promedio(z->hist_NO2, DIAS);
    float mPM = promedio(z->hist_PM25, DIAS);
    float mSO2 = promedio(z->hist_SO2, DIAS);

    const char* promCO = clasificar(mCO, LIM_CO);
    const char* promNO2 = clasificar(mNO2, LIM_NO2);
    const char* promPM = clasificar(mPM, LIM_PM25);
    const char* promSO2 = clasificar(mSO2, LIM_SO2);

    print_both("\n=== Zona: %s ===\n", z->nombre);
    print_both("Actual: CO=%.1f (%s), NO2=%.1f (%s), PM2.5=%.1f (%s), SO2=%.1f (%s)\n",
        z->act_CO, cCO, z->act_NO2, cNO2, z->act_PM25, cPM, z->act_SO2, cSO2);
    print_both("Promedios 30 dias: CO=%.1f (%s), NO2=%.1f (%s), PM2.5=%.1f (%s), SO2=%.1f (%s)\n",
        mCO, promCO, mNO2, promNO2, mPM, promPM, mSO2, promSO2);

    // Recomendaciones según los límites
    print_both("Recomendaciones:\n");
    if (z->act_CO > LIM_CO)
        print_both("- CO elevado: Limite actividades al aire libre y ventile los espacios cerrados.\n");
    if (z->act_NO2 > LIM_NO2)
        print_both("- NO2 elevado: Evite zonas de tráfico y utilice mascarilla si es sensible.\n");
    if (z->act_PM25 > LIM_PM25)
        print_both("- PM2.5 elevado: Personas vulnerables deben permanecer en interiores.\n");
    if (z->act_SO2 > LIM_SO2)
        print_both("- SO2 elevado: Cierre ventanas y evite ejercicios intensos al aire libre.\n");
    if (z->act_CO <= LIM_CO && z->act_NO2 <= LIM_NO2 && z->act_PM25 <= LIM_PM25 && z->act_SO2 <= LIM_SO2)
        print_both("- Todos los contaminantes dentro de los límites recomendados.\n");

    print_both("Prediccion 7 dias (ug/m3):\n");
    print_both("Referencias (max): CO=%.0f, NO2=%.0f, PM2.5=%.1f, SO2=%.0f\n", LIM_CO, LIM_NO2, LIM_PM25, LIM_SO2);
    print_both(" Dia |   CO   |  NO2  | PM2.5 |  SO2\n");
    print_both("-----|--------|-------|--------|--------\n");
    for (int i = 0; i < FUT; ++i)
        print_both("  %2d  | %6.1f | %5.1f | %6.1f | %6.1f\n", i + 1,
            z->pred_CO[i], z->pred_NO2[i], z->pred_PM25[i], z->pred_SO2[i]);
}

int main() {
    FILE* out = fopen("resumen.txt", "w");
    if (!out) { puts("No se pudo crear resumen.txt"); return 1; }

    Zona z[ZONAS];
    for (int i = 0; i < ZONAS; ++i) z[i].nombre = nombres_zonas[i];

    float temp_CO[ZONAS][DIAS] = {0}, temp_NO2[ZONAS][DIAS] = {0}, temp_PM25[ZONAS][DIAS] = {0}, temp_SO2[ZONAS][DIAS] = {0};
    float pred_CO[ZONAS][FUT] = {0}, pred_NO2[ZONAS][FUT] = {0}, pred_PM25[ZONAS][FUT] = {0}, pred_SO2[ZONAS][FUT] = {0};

    int datos_cargados = 0, lecturas_ingresadas = 0;
    int opcion;
    do {
        printf("\n=== MENU PRINCIPAL ===\n");
        printf("1. Cargar datos historicos y predicciones\n");
        printf("2. Ingresar lecturas actuales\n");
        printf("3. Mostrar prediccion lineal multiple\n");
        printf("4. Mostrar resumen por zona\n");
        printf("5. Salir\n");
        printf("Seleccione una opcion: ");
        scanf("%d", &opcion);
        while(getchar()!='\n'); // Limpiar buffer

        switch(opcion) {
            case 1:
                leer_csv("CO.csv", z, temp_CO, 1);
                leer_csv("NO2.csv", z, temp_NO2, 2);
                leer_csv("PM25.csv", z, temp_PM25, 3);
                leer_csv("SO2.csv", z, temp_SO2, 4);

                leer_pred("Pred_CO.csv", z, pred_CO, 1);
                leer_pred("Pred_NO2.csv", z, pred_NO2, 2);
                leer_pred("Pred_PM25.csv", z, pred_PM25, 3);
                leer_pred("Pred_SO2.csv", z, pred_SO2, 4);

                datos_cargados = 1;
                printf("Datos cargados correctamente.\n");
                break;
            case 2:
                pedir_lecturas(z);
                lecturas_ingresadas = 1;
                break;
            case 3: {
                FILE *ft = fopen("Temperatura.csv", "r");
                FILE *fv = fopen("Viento.csv", "r");
                FILE *fh = fopen("Humedad.csv", "r");
                if (!ft || !fv || !fh) {
                    print_both("Advertencia: No se pudieron abrir los archivos meteorologicos. Se mostraran valores por defecto.\n");
                } else {
                    imprimir_pred_lineal(ft, fv, fh, out, z);
                    fclose(ft); fclose(fv); fclose(fh);
                }
                break;
            }
            case 4:
                if (!lecturas_ingresadas)
                    printf("Advertencia: No se han ingresado lecturas actuales. Se mostraran valores por defecto.\n");
                for (int i = 0; i < ZONAS; ++i)
                    procesar(&z[i], out);
                printf("Resumen generado en 'resumen.txt'.\n");
                break;
            case 5:
                printf("Saliendo...\n");
                break;
            default:
                printf("Opcion invalida.\n");
        }
    } while(opcion != 5);

    fclose(out);
    return 0;
}
