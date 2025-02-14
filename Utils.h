/*
 File: Utils.h
 Created on: 28/01/2025
 Author: Felix de las Pozas Alvarez

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _UTILS_H_
#define _UTILS_H_

// Qt
#include <QLabel>

// C++
#include <vector>

/** \class ClickableHoverLabel
 * \brief ClickableLabel subclass that changes the mouse cursor when hovered.
 *
 */
class ClickableHoverLabel
: public QLabel
{
    Q_OBJECT
  public:
    /** \brief ClickableHoverLabel class constructor.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \f Widget flags.
     *
     */
    explicit ClickableHoverLabel(QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief ClickableHoverLabel class constructor.
     * \param[in] text Label text.
     * \param[in] parent Raw pointer of the widget parent of this one.
     * \f Widget flags.
     *
     */
    explicit ClickableHoverLabel(const QString &text, QWidget *parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());

    /** \brief ClickableHoverLabel class virtual destructor.
     *
     */
    virtual ~ClickableHoverLabel();

  signals:
    void clicked();

  protected:
    void mousePressEvent(QMouseEvent *e) override
    {
      emit clicked();
      QLabel::mousePressEvent(e);
    }

    virtual void enterEvent(QEnterEvent *event) override
    {
      setCursor(Qt::PointingHandCursor);
      QLabel::enterEvent(event);
    }

    virtual void leaveEvent(QEvent *event) override
    {
      setCursor(Qt::ArrowCursor);
      QLabel::leaveEvent(event);
    }
};

/** \brief Circular buffer to soften a value.
 * \param[in] T Some sort of numerical value with + and / operations.
 *
 */
template <class T>
class CircularBuffer
{
  public:
    /** \brief Circular buffer class constructor.
     *
     */
    explicit CircularBuffer()
    {}

    /** \brief Add a value to the buffer. 
     * \param[in] value T value.
     *
     */
    void add(const T &value)
    {
      if(!init) initBuffer(value);
      m_data[m_pointer] = value;
      incrementPointer(m_pointer);
    }

    /** \brief Return the softened value for the current buffer.
     *
     */
    T value() const;

  private:
    const int BUFFER_SIZE = 5;

    /** \brief Helper method that increments the given circular buffer pointer. 
     * \param[inout] pointer buffer index.
     *
     */
    void incrementPointer(int &pointer) const
    { auto other = ++pointer % BUFFER_SIZE; pointer = other; }

    /** \brief Helper method that decrements the given circular buffer pointer. 
     * \param[inout] pointer buffer index.
     *
     */
    void decrementPointer(int &pointer) const
    { --pointer; if(pointer < 0) pointer = BUFFER_SIZE-1; }

    /** \brief Initializes the data buffer with the given value.
     * \param[in] initVal Initial buffer value.
     *
     */
    void initBuffer(const T &initVal)
    {
      if(!init) m_data = std::vector<T>(BUFFER_SIZE, initVal);
      init = true;
    }

    std::vector<T> m_data = std::vector<T>(BUFFER_SIZE, T()); /** Circular data buffer. */
    int m_pointer = 0;                       /** Circular current element pointer. */
    bool init = false;                       /** true if initialized and false otherwise. */
};

//---------------------------------------------------------------------
template <class T>
inline T CircularBuffer<T>::value() const
{
  auto pointer = m_pointer;
  int addedValue = 5;
  int accumulation = 0;
  T resultValue;
  for(int i = 0; i < BUFFER_SIZE; ++i)  
  {
    decrementPointer(pointer);
    const auto multiplier = std::max(1, addedValue--);
    accumulation += multiplier;
    resultValue += multiplier * m_data[pointer];
  }
     
  return resultValue / accumulation;
}

#endif // UTILS_H_