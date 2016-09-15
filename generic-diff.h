/*
 * Copyright (c) 2015-2016 SUSE Linux GmbH
 *
 * Licensed under the GNU General Public License Version 2
 * as published by the Free Software Foundation.
 *
 * See http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * for details.
 *
 * Author: Joerg Roedel <jroedel@suse.de>
 */

#ifndef __GENERICDIFF_H
#define __GENERICDIFF_H

#include <memory>

namespace diff {

	using size_type = unsigned;

	enum class diff_type {
		EQUAL,
		ADDED,
		REMOVED,
	};

	struct diff_element {
		enum diff_type	type;
		size_type	idx_a;
		size_type	idx_b;
	};

	// Interface for diffable content,
	// T must provide '==' and '!=' operators
	template<typename T>
	class diffable {
	public:

		// Get total number of elements to diff
		virtual size_type elements() const = 0;

		// Get specific element
		virtual const T&  element(size_type) const = 0;
	};

	class lcs_matrix {
	private:
		size_type		m_x;
		size_type		m_y;

		std::unique_ptr<int[]>	m_matrix;
		std::unique_ptr<bool[]>	m_bool;

	public:
		lcs_matrix(size_type x, size_type y)
			: m_x(x + 1), m_y(y + 1),
			  m_matrix(new int[m_x * m_y]), m_bool(new bool[m_x * m_y])
		{
		}

		void set(size_type x, size_type y, int value)
		{
			m_matrix[(y * m_x) + x] = value;
		}

		int get(size_type x, size_type y) const
		{
			return m_matrix[(y * m_x) + x];
		}

		void set_bool(size_type x, size_type y, bool value)
		{
			m_bool[(y * m_x) + x] = value;
		}

		bool get_bool(size_type x, size_type y) const
		{
			return m_bool[(y * m_x) + x];
		}
	};

	template<typename T>
	class diff {
	private:
		lcs_matrix		m_lcs;
		const diffable<T>	&m_a;
		const diffable<T>	&m_b;

		void create()
		{
			auto size_a = m_a.elements() + 1;
			auto size_b = m_b.elements() + 1;
			decltype(size_a) a;
			decltype(size_b) b;

			for (a = 0; a < size_a; ++a) {

				for (b = 0; b < size_b; ++b) {

					if (a == 0 || b == 0) {
						m_lcs.set(a, b, 0);
						continue;
					}

					if (m_a.element(a - 1) == m_b.element(b - 1)) {
						m_lcs.set(a, b, m_lcs.get(a - 1, b - 1) + 1);
						m_lcs.set_bool(a, b, true);
					} else {
						int ai = m_lcs.get(a - 1, b);
						int bi = m_lcs.get(a, b - 1);

						m_lcs.set(a, b, std::max(ai, bi));
						m_lcs.set_bool(a, b, false);
					}
				}
			}
		}

		void create_diff(std::vector<diff_element> &output,
				 size_type a, size_type b) const
		{
			struct diff_element element;

			if (a > 0 && b > 0 && m_lcs.get_bool(a, b)) {
				create_diff(output, a - 1, b - 1);

				element.type  = diff_type::EQUAL;
				element.idx_a = a - 1;
				element.idx_b = b - 1;

				output.push_back(element);
			} else if (b > 0 && (a == 0 || m_lcs.get(a, b - 1) >= m_lcs.get(a - 1, b))) {
				create_diff(output, a, b - 1);

				element.type  = diff_type::ADDED;
				element.idx_b = b - 1;

				output.push_back(element);
			} else if (a > 0 && (b == 0 || m_lcs.get(a, b - 1) < m_lcs.get(a - 1, b))) {
				create_diff(output, a - 1, b);

				element.type  = diff_type::REMOVED;
				element.idx_a = a - 1;

				output.push_back(element);
			}
		}

	public:
		diff(const diffable<T> &a, const diffable<T> &b)
			: m_lcs(a.elements(), b.elements()), m_a(a), m_b(b)
		{
			create();
		}

		bool is_different() const
		{
			auto size_a = m_a.elements();
			auto size_b = m_b.elements();

			return ((size_a != size_b) || (m_lcs.get(size_a, size_b) != size_a));
		}

		std::vector<diff_element> get_diff() const
		{
			std::vector<diff_element> ret;

			create_diff(ret, m_a.elements(), m_b.elements());

			return ret;
		}
	};

} // Namespace diff

#endif
