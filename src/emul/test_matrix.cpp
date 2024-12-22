#include "../libraries/Matrix_MAX7219/Matrix_MAX7219.h"
#include "../libraries/Matrix_MAX7219/fonts/font_6p.h"
#include "simul_matrix.h"
#include <iostream>

void digitalWrite(int, int ) {}
int digitalRead(int) {return 0;}
int analogRead(int ) {return 0;}
void pinMode(int , int) {}


Matrix_MAX7219::Bitmap<32,8> framebuffer;
using Style = Matrix_MAX7219::TextOutputDef<Matrix_MAX7219::Transform::none, Matrix_MAX7219::BitOp::flip>;

int main() {
    SimulMatrixMAX7219<4> sim;
    sim.current_instance = &sim;

    Matrix_MAX7219::MatrixDriver<4,1,Matrix_MAX7219::ModuleOrder::right_to_left, Matrix_MAX7219::Transform::none> driver;
    driver.begin(0, 1, 2);
    framebuffer.draw_box(0, 0, 15,  7, true);
    Style::textout(framebuffer, Matrix_MAX7219::font_6p, Matrix_MAX7219::Point{0,0}, "12@xyz");
    driver.display(framebuffer, 0, 0, false);
    std::string ln;

    for (int i = 0; i < sim.parts(); ++i) {
        sim.draw_part(i, ln);
        std::cout << ln << std::endl;
    }

}
