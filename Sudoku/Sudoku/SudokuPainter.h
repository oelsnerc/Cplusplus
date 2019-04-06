#include "Painter.h"
#include "SudokuCells.h"

extern const WCHAR* toString(size_t n);

class SudokuPainter : public mmc::Painter
{
private:
    const SudokuCells& ivCells;

protected:
    // Inherited via Painter
    virtual LRESULT do_paint(HDC& hdc, const RECT& rect) override;

public:
    explicit SudokuPainter(const SudokuCells& cells) :
        ivCells(cells)
    { }

    const SudokuCell* getCellAt(LONG x, LONG y, const RECT& rect) const;
};
